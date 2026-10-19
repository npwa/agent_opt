import json

import numpy as np
import pytest

from harness import trace
from harness.telemetry import GpuSamples

T0 = 1000.0


def ms(t):
    return int(round(t * 1000))


def events(steps):
    """steps: list of (start, finish, [(tool, start, end, status)])"""
    out = []
    for start, finish, tools in steps:
        out.append({"type": "step_start", "timestamp": ms(start), "sessionID": "s", "part": {}})
        for name, a, b, st in tools:
            out.append({"type": "tool_use", "timestamp": ms(b), "sessionID": "s",
                        "part": {"tool": name, "state": {"status": st, "time": {"start": ms(a), "end": ms(b)}}}})
        out.append({"type": "step_finish", "timestamp": ms(finish), "sessionID": "s",
                    "part": {"reason": "stop", "tokens": {"input": 1, "output": 1}}})
    return "\n".join(json.dumps(e) for e in out) + "\n"


def proxy_row(start, ttft, total, prompt, completion, chunk_t, attempt="1", trial="t1"):
    return {"task": "k", "trial_id": trial, "attempt": attempt, "wall_start": start, "ttft_s": ttft,
            "total_s": total, "prompt_tokens": prompt, "completion_tokens": completion,
            "input_tok_s": 111.0, "output_tok_s": 22.0, "chunks": len(chunk_t),
            "chunk_t": chunk_t, "status": 200}


def gpu_samples():
    """10 Hz from T0 to T0+30; busy (90 %) during the two LLM windows, otherwise idle."""
    rows = []
    for i in range(301):
        busy = 5 <= i < 60 or 150 <= i < 200
        rows.append({"t": T0 + i / 10, "util": 90.0 if busy else 0.0,
                     "power": 200.0 if busy else 30.0, "mem": 8000.0})
    return GpuSamples(rows)


@pytest.fixture
def parts(tmp_path):
    ev = tmp_path / "events.jsonl"
    ev.write_text(events([
        (T0 + 2.5, T0 + 6.6, [("bash", T0 + 6.0, T0 + 6.5, "completed"),
                              ("read", T0 + 6.5, T0 + 6.6, "error")]),
        (T0 + 16.0, T0 + 20.0, []),
    ]))
    rows = [
        proxy_row(T0 + 0.5, 2.0, 5.5, 1000, 100, [2.0, 2.1, 2.2, 2.4, 5.5]),
        proxy_row(T0 + 15.0, 1.0, 5.0, 1100, 50, [1.0, 1.1, 5.0]),
        proxy_row(T0 + 99.0, 1.0, 5.0, 1, 1, [1.0], attempt="2"),   # other attempt: ignored
        proxy_row(T0 + 99.0, 1.0, 5.0, 1, 1, [1.0], trial="other"),  # other trial: ignored
    ]
    m = tmp_path / "metrics.jsonl"
    m.write_text("\n".join(json.dumps(r) for r in rows) + "\n")
    record = {"trial_id": "t1", "task": "k", "config": "c", "status": "pass", "attempt": 1,
              "wall_start": T0, "wall_end": T0 + 30.0}
    return tmp_path, record, ev, m


def build(parts, samples=True):
    _, record, ev, m = parts
    return trace.build_trace(record, trace.load_event_steps(ev),
                             trace.load_proxy_rows(m, "t1", 1), gpu_samples() if samples else None)


def test_percentile_matches_numpy():
    data = [3.1, 0.2, 9.9, 4.4, 4.4, 1.0, 7.7]
    for q in (0, 25, 50, 90, 99, 100):
        assert trace.percentile(data, q) == pytest.approx(float(np.percentile(data, q)))
    assert trace.percentile([], 50) is None


def test_trace_reproduces_hand_built_timeline(parts):
    t = build(parts)
    assert t["warnings"] == []
    s1, s2 = t["steps"]
    assert (s1["step"], s1["llm_call_id"]) == (1, "t1#a1:1")
    assert s1["first_token_skew_s"] == pytest.approx(0.0)
    assert (s1["t_start"], s1["t_end"], s1["ttft_s"], s1["llm_s"]) == (T0 + 0.5, T0 + 6.0, 2.0, 5.5)
    assert s1["decode_s"] == pytest.approx(3.5)
    assert (s1["prefill_tokens"], s1["decode_tokens"], s1["n_chunks"]) == (1000, 100, 5)
    # ITL from chunk arrivals [2.0, 2.1, 2.2, 2.4, 5.5] -> deltas 100,100,200,3100 ms
    assert s1["itl_p50_ms"] == pytest.approx(150.0)
    assert s1["itl_p99_ms"] == pytest.approx(float(np.percentile([100, 100, 200, 3100], 99)))
    assert s1["tool_names"] == ["bash", "read"] and s1["tool_errors"] == 1
    assert s1["tool_latency_s"] == pytest.approx(0.6)
    assert s1["gap_s"] == pytest.approx(9.0)  # 1006.0 -> next LLM call at 1015.0
    assert s1["gpu_busy_pct_llm"] == pytest.approx(90.0) and s1["gpu_samples_llm"] == 55
    assert s1["gpu_power_w_llm"] == pytest.approx(200.0)
    assert s1["gpu_busy_pct_gap"] == pytest.approx(0.0) and s1["gpu_samples_gap"] == 90
    assert s1["idle_gap_s"] == pytest.approx(9.0)
    # last step: gap runs to trial end
    assert (s2["t_start"], s2["t_end"], s2["gap_s"]) == (T0 + 15.0, T0 + 20.0, pytest.approx(10.0))
    assert s2["tool_names"] == [] and s2["tool_latency_s"] == 0.0
    summ = t["summary"]
    assert summ["n_steps"] == 2 and summ["wall_s"] == pytest.approx(30.0)
    assert summ["llm_s"] == pytest.approx(10.5) and summ["ttft_s"] == pytest.approx(3.0)
    assert summ["tool_s"] == pytest.approx(0.6) and summ["gap_s"] == pytest.approx(19.0)
    assert summ["prefill_tokens"] == 2100 and summ["decode_tokens"] == 150
    assert summ["gpu_telemetry"] is True


def test_trace_without_gpu_samples(parts):
    t = build(parts, samples=False)
    assert t["steps"][0]["gpu_busy_pct_llm"] is None and t["steps"][0]["idle_gap_s"] is None
    assert t["summary"]["gpu_telemetry"] is False and t["summary"]["idle_gap_s"] == 0.0


def test_count_mismatch_and_misalignment_are_reported(parts, tmp_path):
    _, record, ev, m = parts
    steps = trace.load_event_steps(ev)
    rows = trace.load_proxy_rows(m, "t1", 1)
    t = trace.build_trace(record, steps + steps[:1], rows, None)
    assert any(w.startswith("count_mismatch") for w in t["warnings"])
    shifted = [dict(rows[0], wall_start=rows[0]["wall_start"] + 10), rows[1]]
    t = trace.build_trace(record, steps, shifted, None)
    assert any("differs from proxy first token" in w for w in t["warnings"])


def test_load_helpers_tolerate_garbage_and_missing(tmp_path):
    p = tmp_path / "e.jsonl"
    p.write_text('not json\n{"type":"tool_use","timestamp":1,"part":{}}\n')
    assert trace.load_event_steps(p) == []  # tool_use before any step_start is dropped
    assert trace.load_event_steps(tmp_path / "none") == []
    assert trace.load_proxy_rows(tmp_path / "none", "t", 1) == []


def test_build_all_writes_trace_next_to_record(parts, tmp_path):
    base, record, ev, m = parts
    run = tmp_path / "run"
    d = run / "trials" / "t1"
    d.mkdir(parents=True)
    (d / "record.json").write_text(json.dumps(record))
    (d / "events.jsonl").write_text(ev.read_text())
    (run / "proxy").mkdir()
    (run / "proxy" / "metrics.jsonl").write_text(m.read_text())
    failed = run / "trials" / "t2"
    failed.mkdir()
    (failed / "record.json").write_text(json.dumps({"trial_id": "t2", "status": "harness_failed"}))
    (run / "trials" / "t3").mkdir()  # unfinished: no record
    out = trace.build_all(run)
    assert [t["trial_id"] for t in out] == ["t1"]
    saved = json.loads((d / "trace.json").read_text())
    assert saved["summary"]["n_steps"] == 2 and saved["summary"]["gpu_telemetry"] is False


def test_real_recorded_trial_replays_without_gpu_or_model(tmp_path):
    """Replay (F15): a real coding_05 trial recorded on the RTX 3080 (events, proxy, NVML)."""
    from tests.test_verifiers import ROOT

    src = ROOT / "fixtures/traces/coding_05_real"
    traces = trace.build_all(src, gpu_csv=src / "gpu_samples.csv")
    assert len(traces) == 1
    t = traces[0]
    (src / "trials" / t["trial_id"] / "trace.json").unlink()  # keep the fixture directory pristine
    assert t["warnings"] == []
    s = t["steps"][0]
    assert t["summary"]["n_steps"] == 1 and abs(s["first_token_skew_s"]) < 0.5
    assert s["prefill_tokens"] > 5000 and s["decode_tokens"] > 50
    assert 30 < s["gpu_busy_pct_llm"] < 100 and s["gpu_samples_llm"] > 100
    assert abs(s["gpu_samples_llm"] / s["llm_s"] - 10) < 1.0  # ~10 Hz sampling
    assert t["summary"]["gap_s"] < 2.0
