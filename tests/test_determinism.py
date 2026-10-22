import json

from analysis import determinism as det


def events(path, steps, scratch):
    lines = []
    for s in steps:
        if s[0] == "text":
            lines.append({"type": "text", "timestamp": 1, "part": {"text": s[1]}})
        else:
            lines.append({"type": "tool_use", "timestamp": 2, "part": {"tool": s[1], "state": {"input": {"filePath": f"{scratch}/work/{s[2]}"}}}})
    path.write_text("\n".join(json.dumps(x) for x in lines))


def make_run(tmp_path, repeats):
    """repeats: list of (steps, completion_tokens per call, passed)."""
    (tmp_path / "proxy").mkdir()
    metrics = []
    for i, (steps, toks, passed) in enumerate(repeats):
        tid = f"cfg__task_a__{i:02d}"
        d = tmp_path / "trials" / tid
        d.mkdir(parents=True)
        (d / "record.json").write_text(json.dumps({"trial_id": tid, "task": "task_a", "instrumented": True, "status": "pass" if passed else "fail",
                                                   "verdict_passed": passed}))
        events(d / "events.jsonl", steps, f"/tmp/agent_opt_scratch/{tid}")
        metrics += [{"trial_id": tid, "wall_start": 100 * i + j, "status": 200, "completion_tokens": n} for j, n in enumerate(toks)]
    (tmp_path / "proxy" / "metrics.jsonl").write_text("\n".join(json.dumps(m) for m in metrics) + "\n")
    return tmp_path


A = [("tool", "read", "a.py"), ("text", "fixed it"), ("tool", "edit", "a.py")]
B = [("tool", "read", "a.py"), ("text", "fixed it!"), ("tool", "edit", "a.py")]


def test_scratch_path_does_not_make_repeats_different(tmp_path):
    run = make_run(tmp_path, [(A, [10, 20, 30], True), (A, [10, 20, 30], True)])
    r = det.analyse_run(run)["tasks"]["task_a"]
    assert r["identical_to_modal"] == 2 and r["distinct_signatures"] == 1 and r["first_divergence_steps"] == []
    assert r["call_token_agreement"] == 1.0 and r["verdicts_agree"]


def test_divergence_step_token_agreement_and_verdicts(tmp_path):
    run = make_run(tmp_path, [(A, [10, 20, 30], True), (A, [10, 20, 30], True), (B, [10, 25, 30], False)])
    res = det.analyse_run(run)
    r = res["tasks"]["task_a"]
    assert r["identical_to_modal"] == 2 and r["distinct_signatures"] == 2 and r["first_divergence_steps"] == [1]
    assert r["calls_compared"] == 6 and r["calls_with_equal_tokens"] == 5 and not r["verdicts_agree"]
    assert res["call_token_agreement"] == 5 / 6 and res["meets_90pct_rule"] is False
    assert "NOT met" in det.render("x", res) and "| task_a | 3 | 2 | 2 | 1 |" in det.render("x", res)


def test_first_divergence_handles_length_differences():
    assert det.first_divergence([1, 2, 3], [1, 2, 3]) is None
    assert det.first_divergence([1, 2], [1, 2, 3]) == 2
    assert det.first_divergence([1, 9], [1, 2]) == 1


def test_cli_writes_reports(tmp_path, monkeypatch):
    import sys
    run = make_run(tmp_path, [(A, [1], True), (A, [1], True)])
    monkeypatch.setattr(sys, "argv", ["x", "--run", str(run)])
    assert det.main() == 0
    assert (run / "determinism.md").exists() and json.loads((run / "determinism.json").read_text())["meets_90pct_rule"]
