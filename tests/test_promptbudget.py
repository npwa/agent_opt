import json

import pytest

from harness import promptbudget as pb


def _rec(trial, task, t, prompt, ttft):
    return {"trial_id": trial, "task": task, "attempt": "1", "wall_start": t, "status": 200,
            "prompt_tokens": prompt, "ttft_s": ttft, "completion_tokens": 10}


@pytest.fixture
def metrics(tmp_path):
    rows = []
    for i, task in enumerate(["a", "a", "b"]):  # two trials of task a, then b; all cold at 250 tok/s
        t0 = 1000 * i
        rows += [_rec(f"t{i}", task, t0, 10000, 40.0),
                 _rec(f"t{i}", task, t0 + 50, 10400, 1.0 + 400 / 250),  # +400 new tokens
                 _rec(f"t{i}", task, t0 + 60, 12300, 1.0 + 1900 / 250)]  # +1900 new tokens
    rows.append({"task": "x", "status": 200, "prompt_tokens": 5, "ttft_s": 1.0, "wall_start": 5})  # no trial id
    p = tmp_path / "metrics.jsonl"
    p.write_text("\n".join(json.dumps(r) for r in rows) + "\n")
    return p


def test_cold_rate_and_followup_model(metrics):
    calls = pb.load_calls(metrics)
    assert len(calls) == 3 and all(len(v) == 3 for v in calls.values())
    m = pb.call_model(calls)
    assert m["cold_rate_tok_s"]["median"] == pytest.approx(250)
    assert m["followup_overhead_s"] == pytest.approx(1.0)
    assert m["followup_model_median_abs_err_s"] == pytest.approx(0, abs=1e-9)
    assert m["first_call_ttft_total_s"] == pytest.approx(120)
    assert 0.9 < m["followup_cached_share_of_prompt_median"] <= 1.0


def test_no_cross_trial_reuse_detected(metrics):
    r = pb.cross_trial_reuse(pb.load_calls(metrics))
    assert r["fraction_of_first_calls_within_5pct_of_cold"] == 1.0
    assert r["rate_vs_cold_median_ratio_other_task"] == pytest.approx(1.0)
    assert r["n_same_task_as_previous"] == 1


def test_cross_trial_reuse_detected(tmp_path):
    rows = [_rec("t0", "a", 0, 10000, 40.0), _rec("t1", "a", 100, 10000, 10.0)]  # second call 4x faster
    p = tmp_path / "m.jsonl"
    p.write_text("\n".join(json.dumps(r) for r in rows))
    r = pb.cross_trial_reuse(pb.load_calls(p))
    assert r["rate_vs_cold_median_ratio_same_task_as_previous"] > 1.5


SYSTEM = "BASE" * 100 + "<env>\n  cwd /x\n</env>\n" + "Skills provide specialized instructions\n<available_skills><skill>a</skill><skill>b</skill></available_skills>"


def test_split_system():
    parts = pb.split_system(SYSTEM)
    assert parts["base_prompt"] == 400
    assert parts["environment"] == SYSTEM.index("Skills") - 400
    assert parts["skills_block"] == len(SYSTEM) - SYSTEM.index("Skills")
    assert sum(parts.values()) == len(SYSTEM)
    assert pb.split_system("plain") == {"base_prompt": 5}


def test_component_budget_apportions_tokens():
    body = {"messages": [{"role": "system", "content": SYSTEM}, {"role": "user", "content": "hello there"}],
            "tools": [{"function": {"name": "bash", "description": "x" * 100}},
                      {"function": {"name": "read", "description": "y" * 50}}]}
    c = pb.component_budget(body, 1000)
    assert c["skills_listed"] == 2 and c["tools"] == 2
    assert sum(g["tokens_est"] for g in c["groups"]) == pytest.approx(1000, abs=3)
    assert sum(g["share"] for g in c["groups"]) == pytest.approx(1.0)
    names = {r["component"] for r in c["components"]}
    assert {"tool:bash", "tool:read", "system:skills_block", "user_and_history"} <= names


def test_cli_writes_report(metrics, tmp_path):
    run = tmp_path / "run"
    (run / "proxy").mkdir(parents=True)
    (run / "proxy" / "metrics.jsonl").write_text(metrics.read_text())
    req = tmp_path / "req.json"
    req.write_text(json.dumps({"messages": [{"role": "system", "content": SYSTEM}], "tools": []}))
    import sys
    argv = sys.argv
    sys.argv = ["x", "--run", str(run), "--request", str(req)]
    try:
        assert pb.main() == 0
    finally:
        sys.argv = argv
    assert "Cold prefill rate" in (run / "promptbudget.md").read_text()
    assert json.loads((run / "promptbudget.json").read_text())["components"]["skills_listed"] == 2
