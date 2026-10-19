import csv
import time

from harness.telemetry import COLUMNS, GpuSampler, GpuSamples


class FakeBackend:
    def __init__(self, fail_after=None):
        self.n = 0
        self.closed = False
        self.fail_after = fail_after

    def read(self):
        self.n += 1
        if self.fail_after and self.n > self.fail_after:
            raise RuntimeError("nvml gone")
        return {"gpu_util_pct": self.n % 100, "mem_util_pct": 1, "mem_used_mb": 8000,
                "power_w": 100.5, "sm_clock_mhz": 1700, "mem_clock_mhz": 9000,
                "pcie_tx_kbs": 5, "pcie_rx_kbs": 6}

    def close(self):
        self.closed = True


def test_sampler_writes_header_rows_and_closes_backend(tmp_path):
    b = FakeBackend()
    s = GpuSampler(tmp_path / "g.csv", hz=50, backend=b)
    with s:
        time.sleep(0.6)
    rows = list(csv.DictReader(open(tmp_path / "g.csv")))
    assert list(rows[0].keys()) == COLUMNS
    assert 15 <= len(rows) <= 40  # 50 Hz x 0.6 s = 30, tolerant of CI scheduling
    t = [float(r["wall_time"]) for r in rows]
    assert t == sorted(t) and b.closed
    assert s.rows_written == len(rows) and s.error is None
    assert rows[0]["power_w"] == "100.5" and rows[0]["cpu_pct"] != ""


def test_sampler_period_is_close_to_target(tmp_path):
    s = GpuSampler(tmp_path / "g.csv", hz=20, backend=FakeBackend())
    with s:
        time.sleep(1.0)
    t = [float(r["wall_time"]) for r in csv.DictReader(open(tmp_path / "g.csv"))]
    gaps = sorted(b - a for a, b in zip(t, t[1:], strict=False))
    median = gaps[len(gaps) // 2]
    assert abs(median - 0.05) < 0.01


def test_sampler_appends_without_second_header(tmp_path):
    for _ in range(2):
        with GpuSampler(tmp_path / "g.csv", hz=50, backend=FakeBackend()):
            time.sleep(0.1)
    lines = open(tmp_path / "g.csv").read().splitlines()
    assert sum(ln.startswith("wall_time") for ln in lines) == 1


def test_sampler_reports_backend_failure(tmp_path):
    s = GpuSampler(tmp_path / "g.csv", hz=50, backend=FakeBackend(fail_after=3))
    with s:
        time.sleep(0.3)
    assert isinstance(s.error, RuntimeError) and s.rows_written == 3


def test_samples_window_stats_and_empty_window():
    rows = [{"t": 10.0 + i * 0.1, "util": 0.0 if i < 5 else 80.0, "power": 50.0, "mem": 100.0 + i}
            for i in range(10)]
    g = GpuSamples(rows)
    w = g.window(10.0, 10.49)
    assert w.n == 5 and w.mean_util == 0.0 and w.idle_frac == 1.0
    w = g.window(10.0, 10.95)
    assert w.n == 10 and w.mean_util == 40.0 and w.idle_frac == 0.5
    assert w.max_mem_used_mb == 109.0 and w.mean_power_w == 50.0
    empty = g.window(50.0, 60.0)
    assert empty.n == 0 and empty.mean_util is None


def test_from_csv_skips_bad_rows_and_missing_file(tmp_path):
    p = tmp_path / "g.csv"
    p.write_text("wall_time,gpu_util_pct,power_w,mem_used_mb\n1.0,10,50,100\nbad,x,y,z\n2.0,,60,\n")
    g = GpuSamples.from_csv(p)
    assert len(g.rows) == 2 and g.rows[1]["util"] is None
    assert GpuSamples.from_csv(tmp_path / "none.csv").rows == []
