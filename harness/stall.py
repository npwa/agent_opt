"""
harness/stall.py

Stall detection for a running OpenCode process (plan item P13).

A trial hung once with OpenCode waiting for a response from Ollama that never came (GPU idle, no events,
no child process working). That is a harness fault, not a wrong answer, but the runner could only end it
with the task timeout and then recorded it as an outcome. A trial is *stalled* when, for ``stall_s`` seconds,
all of these hold: no new bytes in the event stream, the GPU is idle, and no child process of OpenCode (a
running tool such as a compiler or test) is using the CPU. A long generation keeps the GPU busy and a long
tool keeps a child busy, so neither is mistaken for a stall.
"""

import psutil

GPU_BUSY_PCT = 5.0
CHILD_BUSY_CPU_PCT = 5.0


def gpu_busy_probe():
    """-> callable returning True when the GPU is doing work; always False when NVML is unavailable."""
    try:
        import pynvml
        pynvml.nvmlInit()
        handle = pynvml.nvmlDeviceGetHandleByIndex(0)
    except Exception:  # no NVML: the other signals still apply
        return lambda: False

    def busy() -> bool:
        try:
            return pynvml.nvmlDeviceGetUtilizationRates(handle).gpu > GPU_BUSY_PCT
        except Exception:
            return False
    return busy


_SEEN: dict[int, psutil.Process] = {}  # psutil measures CPU between calls on the same Process object


def children_busy(pid: int) -> bool:
    """True when any descendant of ``pid`` used more than a few percent CPU since the previous call
    (the first call for a process only primes its counters)."""
    try:
        procs = psutil.Process(pid).children(recursive=True)
    except psutil.Error:
        return False
    busy = False
    for c in procs:
        proc = _SEEN.setdefault(c.pid, c)
        try:
            if proc.cpu_percent(interval=None) > CHILD_BUSY_CPU_PCT:
                busy = True
        except psutil.Error:
            _SEEN.pop(c.pid, None)
    for gone in set(_SEEN) - {c.pid for c in procs}:
        _SEEN.pop(gone, None)
    return busy


_GPU = None


def make_probe():
    """-> busy(pid): GPU busy or a child process working (the NVML handle is created once)."""
    global _GPU
    if _GPU is None:
        _GPU = gpu_busy_probe()
    gpu = _GPU
    return lambda pid: gpu() or children_busy(pid)
