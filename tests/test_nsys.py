import json
import sqlite3

import pytest

from harness import nsys
from tests.test_trace import T0, build, parts  # noqa: F401  (parts is a fixture)

SESSION_EPOCH_NS = int((T0 - 10) * 1e9)  # capture started 10 s before the trial


def make_export(path, kernels, launches=(), graphs=()):
    """kernels: [(epoch_start, epoch_end, name)] -> a minimal nsys sqlite export."""
    c = sqlite3.connect(path)
    c.executescript("""
        create table StringIds(id integer primary key, value text);
        create table TARGET_INFO_SESSION_START_TIME(utcEpochNs integer, utcTime text, localTime text,
                                                    systemClockNs integer, sessionStartCntVctNs integer);
        create table CUPTI_ACTIVITY_KIND_KERNEL(start integer, end integer, deviceId integer,
                                                shortName integer, demangledName integer);
        create table CUPTI_ACTIVITY_KIND_RUNTIME(start integer, end integer, nameId integer);
        create table CUPTI_ACTIVITY_KIND_GRAPH_TRACE(start integer, end integer, deviceId integer,
                                                     graphId integer);
    """)
    c.execute("insert into TARGET_INFO_SESSION_START_TIME values (?,?,?,?,?)",
              (SESSION_EPOCH_NS, "", "", 0, None))
    names: dict[str, int] = {}

    def sid(n):
        if n not in names:
            names[n] = len(names) + 1
            c.execute("insert into StringIds values (?,?)", (names[n], n))
        return names[n]

    for s, e, n in kernels:
        c.execute("insert into CUPTI_ACTIVITY_KIND_KERNEL values (?,?,0,?,?)",
                  (int((s - (T0 - 10)) * 1e9), int((e - (T0 - 10)) * 1e9), sid(n), sid(n + "<T>")))
    for s, e in graphs:
        c.execute("insert into CUPTI_ACTIVITY_KIND_GRAPH_TRACE values (?,?,0,1)",
                  (int((s - (T0 - 10)) * 1e9), int((e - (T0 - 10)) * 1e9)))
    for s, e, n in launches:
        c.execute("insert into CUPTI_ACTIVITY_KIND_RUNTIME values (?,?,?)",
                  (int((s - (T0 - 10)) * 1e9), int((e - (T0 - 10)) * 1e9), sid(n)))
    c.commit()
    c.close()


def synthetic_kernels():
    """Trace windows (see test_trace): LLM1 = [T0+0.5, T0+6.0) with TTFT 2.0, LLM2 = [T0+15, T0+20).
    Kernels: 1 ms every 10 ms through LLM1 decode [T0+2.5, T0+6.0) and LLM2 decode [T0+16, T0+20),
    plus a dense burst in LLM1 prefill [T0+0.5, T0+2.5) (80 % busy), nothing in the gap."""
    ks = []
    t = T0 + 0.5
    while t < T0 + 2.5:
        ks.append((t, t + 0.008, "gemm"))
        t += 0.01
    for lo, hi in ((T0 + 2.5, T0 + 6.0), (T0 + 16.0, T0 + 20.0)):
        t = lo
        while t < hi:
            ks.append((t, t + 0.001, "mul_mat_vec"))
            t += 0.01
    return ks


def test_load_kernels_converts_to_epoch_seconds(tmp_path):
    db = tmp_path / "c.sqlite"
    make_export(db, [(T0 + 1.0, T0 + 1.5, "k1"), (T0 + 3.0, T0 + 3.1, "k2")])
    ks = nsys.load_kernels(db)
    assert [k.name for k in ks] == ["k1", "k2"]
    assert ks[0].start == pytest.approx(T0 + 1.0, abs=1e-6) and ks[1].end == pytest.approx(T0 + 3.1)


def test_missing_kernel_table_means_no_kernels(tmp_path):
    db = tmp_path / "c.sqlite"
    c = sqlite3.connect(db)
    c.execute("create table StringIds(id integer, value text)")
    c.commit()
    assert nsys.load_kernels(db) == [] and nsys.load_launch_calls(db) == []


def test_launch_calls_filtered_by_api_name(tmp_path):
    db = tmp_path / "c.sqlite"
    make_export(db, [], [(T0, T0 + 0.001, "cudaLaunchKernel_v7000"), (T0, T0 + 0.002, "cudaMemcpyAsync"),
                         (T0 + 1, T0 + 1.001, "cuGraphLaunch")])
    assert len(nsys.load_launch_calls(db)) == 2


def test_active_s_and_gap_stats():
    iv = [(0.0, 1.0), (1.5, 2.0), (3.0, 3.5)]
    assert nsys.active_s(iv, 0.5, 3.25) == pytest.approx(0.5 + 0.5 + 0.25)
    g = nsys.launch_gap_stats(iv, 0.0, 10.0)
    assert g["n"] == 2 and g["total_s"] == pytest.approx(1.5) and g["max_ms"] == pytest.approx(1000.0)
    assert g["p50_ms"] == pytest.approx(750.0)
    assert nsys.launch_gap_stats([], 0, 1)["p50_ms"] is None


def test_kernel_summary_ranks_by_total_time():
    ks = [nsys.Kernel(0, 0.002, "a"), nsys.Kernel(1, 1.001, "b"), nsys.Kernel(2, 2.001, "b")]
    rows = nsys.kernel_summary(ks)
    assert rows[0]["kernel"] == "a" and rows[0]["total_ms"] == pytest.approx(2.0)
    assert rows[1]["count"] == 2 and sum(r["pct_of_kernel_time"] for r in rows) == pytest.approx(100.0)


def test_alignment_against_hand_built_trace(parts, tmp_path):  # noqa: F811
    db = tmp_path / "c.sqlite"
    make_export(db, synthetic_kernels())
    trace = build(parts)
    al = nsys.analyze(db, trace, tmp_path / "out")
    s1, s2 = al["steps"]
    assert s1["kernel_busy_pct_prefill"] == pytest.approx(80.0, abs=1.0)
    assert s1["kernel_busy_pct_decode"] == pytest.approx(10.0, abs=1.0)
    assert s2["kernel_busy_pct_decode"] == pytest.approx(10.0, abs=1.0)
    sm = al["summary"]
    assert sm["kernel_time_in_llm_windows_pct"] == pytest.approx(100.0, abs=0.5)  # nothing runs in the gaps
    assert sm["decode_slices_with_kernels_pct"] == pytest.approx(100.0)
    assert sm["launch_gaps_run"]["n"] > 100 and sm["launch_gaps_run"]["max_ms"] > 5000  # the 9 s gap
    assert (tmp_path / "out" / "kernel_summary.csv").read_text().startswith("kernel,count")
    assert "GPU-active time inside proxy LLM windows" in (tmp_path / "out" / "nsys_alignment.md").read_text()
    assert json.loads((tmp_path / "out" / "nsys_alignment.json").read_text())["n_kernels"] == len(synthetic_kernels())


def test_kernels_outside_llm_windows_reduce_the_overlap(parts, tmp_path):  # noqa: F811
    db = tmp_path / "c.sqlite"
    stray = [(T0 + 8.0, T0 + 9.0, "stray")]  # 1 s of GPU work between the two LLM calls
    make_export(db, synthetic_kernels() + stray)
    al = nsys.analyze(db, build(parts), tmp_path / "o")
    assert al["summary"]["kernel_time_in_llm_windows_pct"] < 90.0


def test_idle_decode_window_is_reported(parts, tmp_path):  # noqa: F811
    db = tmp_path / "c.sqlite"
    make_export(db, [(T0 + 0.5, T0 + 1.0, "only_prefill")])
    al = nsys.analyze(db, build(parts), tmp_path / "o")
    assert al["summary"]["decode_slices_with_kernels_pct"] == pytest.approx(0.0)


def test_preconditions_report_busy_service_and_ports(monkeypatch):
    calls = {"http://localhost:11434/api/ps": {"models": [{"name": "qwen"}]},
             "http://127.0.0.1:11435/api/tags": {"models": []}}
    monkeypatch.setattr(nsys, "_get", lambda url, timeout=5: calls.get(url))
    probs = nsys.check_preconditions()
    assert any("model loaded" in p for p in probs) and any("11435" in p for p in probs)
    monkeypatch.setattr(nsys, "_get", lambda url, timeout=5: None)
    assert not any("loaded" in p or "port" in p for p in nsys.check_preconditions())


def test_cuda_graph_executions_count_as_gpu_activity(parts, tmp_path):  # noqa: F811
    """llama.cpp decodes with CUDA graphs: they live in GRAPH_TRACE, not in the kernel table."""
    db = tmp_path / "c.sqlite"
    graphs, t = [], T0 + 2.5
    while t < T0 + 6.0:  # 5 ms graph every 10 ms through LLM1's decode window
        graphs.append((t, t + 0.005))
        t += 0.01
    make_export(db, [], graphs=graphs)
    ks = nsys.load_kernels(db)
    assert ks and all(k.name == nsys.GRAPH_NAME for k in ks)
    al = nsys.analyze(db, build(parts), tmp_path / "o")
    assert al["n_kernels"] == 0 and al["n_graph_launches"] == len(graphs)
    assert al["steps"][0]["kernel_busy_pct_decode"] == pytest.approx(50.0, abs=2.0)
    assert al["steps"][0]["kernel_busy_pct_prefill"] == pytest.approx(0.0)
    assert "CUDA graph launches" in (tmp_path / "o" / "nsys_alignment.md").read_text()
