"""
harness/telemetry.py

GPU/CPU/RAM sampler (F8) and sample lookup for the trace join.

NVML is polled at ``hz`` (default 10 Hz) for utilisation, memory, power and SM/memory clocks.
PCIe throughput is slow to read (~40 ms per call: the NVML counter integrates over a window),
so a second thread reads it continuously and the 10 Hz loop records the latest value. CPU and
RAM (psutil) are refreshed at 1 Hz and repeated in between.

Caveat that matters for interpretation: NVML ``utilization.gpu`` is the fraction of the last
driver sampling window (between 1/6 s and 1 s, depending on the product) in which *any* kernel
was running. It is a coarse "was the GPU busy" signal, not instantaneous occupancy, and a value
of 100 % does not mean the SMs were full.

Samples carry epoch wall-clock time (``time.time()``) so they can be joined with proxy records
and OpenCode event timestamps, which are epoch-based too.
"""

import bisect
import csv
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

import psutil

COLUMNS = ["wall_time", "gpu_util_pct", "mem_util_pct", "mem_used_mb", "power_w",
           "sm_clock_mhz", "mem_clock_mhz", "pcie_tx_kbs", "pcie_rx_kbs", "cpu_pct", "ram_pct"]
IDLE_UTIL_PCT = 5.0  # a sample at or below this utilisation counts as "GPU idle"


class GpuBackend(Protocol):
    def read(self) -> dict: ...
    def close(self) -> None: ...


class NvmlBackend:
    def __init__(self, index: int = 0):
        import pynvml  # lazy: CI and replay mode have no NVML

        self.nv = pynvml
        pynvml.nvmlInit()
        self.handle = pynvml.nvmlDeviceGetHandleByIndex(index)
        self._pcie = (None, None)
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._pcie_loop, daemon=True, name="pcie-sampler")
        self._thread.start()

    def _pcie_loop(self) -> None:
        nv = self.nv
        while not self._stop.is_set():
            try:
                tx = nv.nvmlDeviceGetPcieThroughput(self.handle, nv.NVML_PCIE_UTIL_TX_BYTES)
                rx = nv.nvmlDeviceGetPcieThroughput(self.handle, nv.NVML_PCIE_UTIL_RX_BYTES)
                self._pcie = (tx, rx)
            except nv.NVMLError:
                time.sleep(0.5)

    def read(self) -> dict:
        nv, h = self.nv, self.handle
        util = nv.nvmlDeviceGetUtilizationRates(h)
        mem = nv.nvmlDeviceGetMemoryInfo(h)
        return {
            "gpu_util_pct": util.gpu, "mem_util_pct": util.memory,
            "mem_used_mb": mem.used // 2**20,
            "power_w": round(nv.nvmlDeviceGetPowerUsage(h) / 1000.0, 1),
            "sm_clock_mhz": nv.nvmlDeviceGetClockInfo(h, nv.NVML_CLOCK_SM),
            "mem_clock_mhz": nv.nvmlDeviceGetClockInfo(h, nv.NVML_CLOCK_MEM),
            "pcie_tx_kbs": self._pcie[0], "pcie_rx_kbs": self._pcie[1],
        }

    def close(self) -> None:
        self._stop.set()
        self._thread.join(timeout=2)
        self.nv.nvmlShutdown()


class GpuSampler:
    """Background sampler writing one CSV row per tick; use as a context manager."""

    def __init__(self, path: Path, hz: float = 10.0, backend: GpuBackend | None = None,
                 clock=time.time):
        self.path, self.period = Path(path), 1.0 / hz
        self.backend = backend
        self.clock = clock
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self.rows_written = 0
        self.max_lag_s = 0.0
        self.error: Exception | None = None

    def start(self) -> "GpuSampler":
        if self.backend is None:
            self.backend = NvmlBackend()
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._thread = threading.Thread(target=self._run, daemon=True, name="gpu-sampler")
        self._thread.start()
        return self

    def stop(self) -> None:
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=5)
        if self.backend:
            self.backend.close()

    def __enter__(self) -> "GpuSampler":
        return self.start()

    def __exit__(self, *exc) -> None:
        self.stop()

    def _run(self) -> None:
        new = not self.path.exists()
        cpu = ram = 0.0
        last_slow = -1.0
        psutil.cpu_percent(interval=None)
        try:
            with open(self.path, "a", newline="") as f:
                w = csv.writer(f)
                if new:
                    w.writerow(COLUMNS)
                next_t = time.monotonic()
                last_flush = next_t
                while not self._stop.is_set():
                    now_m = time.monotonic()
                    if now_m - last_slow >= 1.0:  # CPU/RAM at 1 Hz
                        cpu, ram = psutil.cpu_percent(interval=None), psutil.virtual_memory().percent
                        last_slow = now_m
                    wall = self.clock()
                    r = self.backend.read()
                    w.writerow([f"{wall:.3f}"] + [r.get(c) for c in COLUMNS[1:-2]] + [cpu, ram])
                    self.rows_written += 1
                    if now_m - last_flush >= 1.0:
                        f.flush()
                        last_flush = now_m
                    next_t += self.period
                    delay = next_t - time.monotonic()
                    if delay < 0:  # fell behind: record lag and resynchronise
                        self.max_lag_s = max(self.max_lag_s, -delay)
                        next_t = time.monotonic()
                        delay = 0
                    self._stop.wait(delay)
        except Exception as e:  # surface sampler death instead of silently losing telemetry
            self.error = e


# ---- reading samples back ------------------------------------------------------------------------

@dataclass
class WindowStats:
    n: int
    mean_util: float | None
    idle_frac: float | None
    mean_power_w: float | None
    max_mem_used_mb: float | None


class GpuSamples:
    def __init__(self, rows: list[dict]):
        self.rows = sorted(rows, key=lambda r: r["t"])
        self.times = [r["t"] for r in self.rows]

    @classmethod
    def from_csv(cls, path: Path) -> "GpuSamples":
        rows = []
        if Path(path).exists():
            with open(path) as f:
                for r in csv.DictReader(f):
                    try:
                        rows.append({"t": float(r["wall_time"]),
                                     "util": _f(r["gpu_util_pct"]), "power": _f(r["power_w"]),
                                     "mem": _f(r["mem_used_mb"])})
                    except (KeyError, ValueError):
                        continue
        return cls(rows)

    def window(self, t0: float, t1: float) -> WindowStats:
        # half-open [t0, t1): a sample on a shared boundary belongs to exactly one of two
        # adjacent windows (LLM call / gap), so nothing is counted twice
        lo, hi = bisect.bisect_left(self.times, t0), bisect.bisect_left(self.times, t1)
        sel = self.rows[lo:hi]
        utils = [r["util"] for r in sel if r["util"] is not None]
        powers = [r["power"] for r in sel if r["power"] is not None]
        mems = [r["mem"] for r in sel if r["mem"] is not None]
        if not utils:
            return WindowStats(0, None, None, None, None)
        return WindowStats(
            len(utils), sum(utils) / len(utils),
            sum(u <= IDLE_UTIL_PCT for u in utils) / len(utils),
            sum(powers) / len(powers) if powers else None, max(mems) if mems else None)


def _f(v: str | None) -> float | None:
    try:
        return float(v)
    except (TypeError, ValueError):
        return None
