import json
from pathlib import Path

import pytest

from harness import decompose, trace
from harness.telemetry import GpuSamples
from tests.test_trace import T0, build, gpu_samples, parts  # noqa: F401  (parts is a fixture)

ROOT = Path(__file__).resolve().parent.parent


def test_merge_and_overlap_len():
    assert decompose.merge([(5, 7), (1, 3), (2, 4), (9, 9)]) == [(1, 4), (5, 7)]
    assert decompose.overlap_len([(1, 4), (5, 7)], 3, 6) == 2.0
    assert decompose.overlap_len([], 0, 10) == 0.0


def test_known_sums_synthetic(parts):  # noqa: F811
    d = decompose.decompose(build(parts), gpu_samples())
    b = d["buckets_s"]
    assert b["prefill"] == pytest.approx(3.0) and b["decode"] == pytest.approx(7.5)
    assert b["tool"] == pytest.approx(0.6)
    assert d["harness_parts_s"] == pytest.approx(
        {"startup": 0.5, "between_calls": 8.4, "shutdown": 10.0})
    assert b["harness"] == pytest.approx(18.9)
    assert sum(b.values()) == pytest.approx(30.0)  # buckets sum to wall-clock
    assert abs(d["buckets_pct"]["unattributed"]) < 1.0 and d["ok"] is True
    assert d["buckets_pct"]["prefill"] == pytest.approx(10.0)
    g = d["gpu"]
    assert g["duty_cycle_pct"] == pytest.approx(35.0, abs=0.5)  # 105 of 300 samples busy
    assert g["util_prefill_pct"] == pytest.approx(90.0) and g["util_decode_pct"] == pytest.approx(90.0)
    assert g["energy_j"] == pytest.approx(35.0 / 100 * 200.0 * 30 + 65.0 / 100 * 30.0 * 30, rel=0.03)
    assert d["first_call"]["ttft_s"] == 2.0 and d["first_call"]["prefill_tok_s"] == 500.0
    assert d["first_call"]["share_of_prefill_pct"] == pytest.approx(100 * 2 / 3)


def test_overlapping_llm_windows_are_flagged_as_unattributed(parts):  # noqa: F811
    t = build(parts)
    t["steps"][1]["t_start"] = T0 + 5.0   # second call starts before the first one ended
    t["steps"][1]["t_end"] = T0 + 10.0
    t["warnings"] = []
    d = decompose.decompose(t)
    assert d["llm_overlap_s"] == pytest.approx(1.0) and d["ok"] is False
    assert d["tool_overlap_s"] == pytest.approx(0.6)  # tools now run inside an LLM window
    assert d["buckets_pct"]["unattributed"] != pytest.approx(0.0, abs=1.0)


def test_opencode_step_without_proxy_record_is_unattributed(parts):  # noqa: F811
    t = build(parts)
    t["unmatched_event_steps"] = [{"start": T0 + 8.0, "finish": T0 + 12.0}]
    d = decompose.decompose(t)
    assert d["unmatched_event_s"] == 4.0
    assert d["buckets_s"]["unattributed"] == pytest.approx(4.0) and d["ok"] is False


def test_parallel_tools_counted_once_and_tail_tools_attributed(parts):  # noqa: F811
    t = build(parts)
    t["steps"][0]["tools"] = [
        {"tool": "a", "start": T0 + 6.0, "end": T0 + 8.0, "status": "completed"},
        {"tool": "b", "start": T0 + 7.0, "end": T0 + 9.0, "status": "completed"},   # overlaps a
    ]
    t["steps"][1]["tools"] = [{"tool": "c", "start": T0 + 21.0, "end": T0 + 22.0, "status": "completed"}]
    d = decompose.decompose(t)
    assert d["buckets_s"]["tool"] == pytest.approx(3.0 + 1.0)  # union 6-9, plus the tail tool
    assert d["harness_parts_s"]["shutdown"] == pytest.approx(10.0 - 1.0)
    assert abs(d["buckets_pct"]["unattributed"]) < 1e-6


def test_no_calls_or_walltime_is_reported_not_crashed(parts):  # noqa: F811
    t = build(parts)
    t["steps"] = []
    assert decompose.decompose(t)["ok"] is False
    t = build(parts)
    t["summary"]["wall_end"] = t["summary"]["wall_start"]
    d = decompose.decompose(t)
    assert d["ok"] is False and d["reason"] == "no wall-clock"


def test_join_warnings_make_a_trial_not_ok(parts):  # noqa: F811
    t = build(parts)
    t["warnings"] = ["count_mismatch: ..."]
    assert decompose.decompose(t)["ok"] is False


def test_aggregate_is_time_weighted_and_markdown(parts):  # noqa: F811
    d = decompose.decompose(build(parts))
    d2 = dict(d, task="other", wall_s=60.0,
              buckets_s={k: v * 2 for k, v in d["buckets_s"].items()})
    agg = decompose.aggregate([d, d2], {"k": "bugfix", "other": "bugfix"})
    a = agg["bugfix"]
    assert a["n"] == 2 and a["wall_s"] == pytest.approx(90.0)
    assert sum(a["pct"].values()) == pytest.approx(100.0)
    assert a["pct"]["prefill"] == pytest.approx(10.0)
    md = decompose.to_markdown(agg)
    assert md.splitlines()[0].startswith("| Category") and "| bugfix | 2 |" in md


@pytest.mark.parametrize("name", ["coding_05_real", "bugfix_01_real"])
def test_real_trials_meet_the_unattributed_acceptance_rule(name):
    """Acceptance (plan M2.4): unattributed < 5 % on real recorded runs."""
    src = ROOT / "fixtures/traces" / name
    traces = trace.build_all(src, gpu_csv=src / "gpu_samples.csv")
    for p in src.glob("trials/*/trace.json"):
        p.unlink()
    assert len(traces) == 1
    d = decompose.decompose(traces[0], GpuSamples.from_csv(src / "gpu_samples.csv"))
    assert d["ok"] is True, d["warnings"]
    assert abs(d["buckets_pct"]["unattributed"]) < decompose.UNATTRIBUTED_LIMIT_PCT
    assert sum(d["buckets_s"].values()) == pytest.approx(d["wall_s"])
    assert d["buckets_pct"]["prefill"] > 30  # fresh 10k-token prompt dominates these short runs
    assert d["gpu"]["duty_cycle_pct"] > 20


def test_decompose_cli_end_to_end(tmp_path, monkeypatch, capsys):
    src = ROOT / "fixtures/traces/bugfix_01_real"
    run = tmp_path / "run"
    import shutil
    shutil.copytree(src, run)
    trace.build_all(run, gpu_csv=run / "gpu_samples.csv")
    monkeypatch.setattr("sys.argv", ["decompose", "--run", str(run), "--tasks", str(ROOT / "tasks")])
    assert decompose.main() == 0
    out = capsys.readouterr().out
    assert "| bugfix | 1 |" in out and "0 flagged" in out
    assert json.loads((run / "decomposition.json").read_text())[0]["ok"] is True
