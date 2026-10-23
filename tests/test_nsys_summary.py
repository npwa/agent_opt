import json

import pytest

from analysis import nsys_summary as ns


def make_capture(root, name, task, wall, ttft, decode, active, pre_pct, dec_pct, nvml=50.0):
    d = root / name
    t = d / "run" / "trials" / "t1"
    t.mkdir(parents=True)
    (t / "trace.json").write_text(json.dumps({
        "task": task, "status": "pass",
        "summary": {"wall_s": wall, "ttft_s": ttft, "decode_s": decode, "n_steps": 1},
        "steps": [{"step": 1, "ttft_s": ttft, "decode_s": decode}]}))
    (t / "record.json").write_text(json.dumps({"status": "pass"}))
    (d / "nsys_alignment.json").write_text(json.dumps({
        "summary": {"kernel_active_s_in_run": active, "nvml_busy_pct_llm": nvml, "kernel_time_in_llm_windows_pct": 100.0},
        "steps": [{"step": 1, "kernel_busy_pct_prefill": pre_pct, "kernel_busy_pct_decode": dec_pct,
                   "launch_gaps_decode": {"n": 10, "p50_ms": 0.7, "p99_ms": 3.0}}]}))
    return d


def test_capture_row_computes_weighted_shares(tmp_path):
    d = make_capture(tmp_path, "a", "coding_05_scientific_lan", wall=100, ttft=40, decode=50, active=10, pre_pct=5, dec_pct=15)
    r = ns.capture_row(d, {"coding_05_scientific_lan": "reasoning"})
    assert r["category"] == "reasoning" and r["other_s"] == pytest.approx(10)
    assert r["gpu_active_pct_of_wall"] == pytest.approx(10.0)
    assert r["gpu_active_pct_of_prefill"] == pytest.approx(5.0) and r["gpu_active_pct_of_decode"] == pytest.approx(15.0)
    assert r["decode_gap_p50_ms"] == 0.7 and r["status"] == "pass"


def test_incomplete_capture_is_skipped(tmp_path):
    (tmp_path / "empty").mkdir()
    assert ns.capture_row(tmp_path / "empty", {}) is None


def test_category_means_and_report(tmp_path, monkeypatch, capsys):
    make_capture(tmp_path, "x1", "t_a", 100, 40, 50, 10, 5, 15)
    make_capture(tmp_path, "x2", "t_a", 200, 100, 80, 30, 10, 20)
    make_capture(tmp_path, "y1", "t_b", 50, 5, 40, 20, 10, 50)
    (tmp_path / "broken").mkdir()
    (tmp_path / "tasks").mkdir()
    for t, c in (("t_a", "bugfix"), ("t_b", "code_gen")):
        (tmp_path / "tasks" / f"{t}.yaml").write_text(f"id: {t}\ncategory: {c}\n")
    import sys
    monkeypatch.setattr(sys, "argv", ["x", "--captures", str(tmp_path), "--out", str(tmp_path / "out"), "--tasks", str(tmp_path / "tasks")])
    assert ns.main() == 0
    summ = json.loads((tmp_path / "out" / "summary.json").read_text())
    cats = {c["category"]: c for c in summ["categories"]}
    assert cats["bugfix"]["captures"] == 2 and cats["bugfix"]["gpu_active_pct_of_wall"] == pytest.approx((10 + 15) / 2)
    assert summ["skipped"] == ["broken", "out"] or "broken" in summ["skipped"]
    text = (tmp_path / "out" / "RESULTS.md").read_text()
    assert "| bugfix | 2 |" in text and "GPU is active" in text
