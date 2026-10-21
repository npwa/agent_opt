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
    assert m["cold_rate_tok_s"] == pytest.approx(250)
    assert m["first_calls_cold"] == 3 and m["first_calls_warm"] == 0
    assert m["followup_overhead_s"] == pytest.approx(1.0)
    assert m["followup_model_median_abs_err_s"] == pytest.approx(0, abs=1e-9)
    assert m["first_call_ttft_total_s"] == pytest.approx(120)
    assert 0.9 < m["followup_cached_share_of_prompt_median"] <= 1.0


def test_no_cross_trial_reuse_detected(metrics):
    r = pb.cross_trial_reuse(pb.load_calls(metrics))
    assert r["fraction_of_first_calls_cold"] == 1.0
    assert r["speedup_vs_cold_median_other_task"] == pytest.approx(1.0)
    assert r["n_same_task_as_previous"] == 1


def test_warm_first_calls_do_not_bias_the_cold_rate(tmp_path):
    rows = []
    for i in range(6):  # most first calls are warm (2.5 s for 10k tokens); follow-ups reveal the true rate
        t0 = 1000 * i
        rows += [_rec(f"t{i}", "a" if i % 2 else "b", t0, 10000, 2.5 if i else 40.0),
                 _rec(f"t{i}", "a" if i % 2 else "b", t0 + 50, 10400, 1.0 + 400 / 250),
                 _rec(f"t{i}", "a" if i % 2 else "b", t0 + 60, 12300, 1.0 + 1900 / 250)]
    p = tmp_path / "m.jsonl"
    p.write_text("\n".join(json.dumps(r) for r in rows))
    calls = pb.load_calls(p)
    assert pb.cold_rate(calls) == pytest.approx(250)
    m = pb.call_model(calls)
    assert m["first_calls_cold"] == 1 and m["first_calls_warm"] == 5
    r = pb.cross_trial_reuse(calls)
    assert r["fraction_of_first_calls_cold"] == pytest.approx(1 / 6)
    assert r["speedup_vs_cold_median_other_task"] > 10


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
