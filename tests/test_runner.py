import json
import os
import stat
import sys
from dataclasses import replace

import pytest

from harness import runner
from harness.taskspec import load_task
from harness.verifiers import TaskVerdict, VerifierResult
from tests.test_verifiers import GOLDEN, ROOT

CFG = runner.RunConfig(id="cfg", model="ollama/m")
CFG2 = runner.RunConfig(id="cfg2", model="ollama/m", seed_mode="none")


def specs(*ids):
    return [load_task(ROOT / "tasks" / f"{i}.yaml") for i in ids]


FIZZ = "coding_02_fizzbuzz_gen"
SQRT = "coding_01_my_sqrt_review"


# ---- configs / plan ---------------------------------------------------------------------------

def test_load_config_ok_and_errors(tmp_path):
    good = tmp_path / "c.yaml"
    good.write_text("id: a_1\nmodel: ollama/x\n")
    assert runner.load_config(good).seed_mode == "per_trial"
    for body in ["id: a\n", "id: a\nmodel: nomodel\n", "id: a\nmodel: p/m\nbogus: 1\n",
                 "id: a\nmodel: p/m\nseed_mode: sometimes\n", "id: 'a b'\nmodel: p/m\n", "- x\n"]:
        bad = tmp_path / "bad.yaml"
        bad.write_text(body)
        with pytest.raises(ValueError):
            runner.load_config(bad)


def test_repo_baseline_config_loads():
    assert runner.load_config(ROOT / "configs/baseline_32k.yaml").model == "ollama/qwen3.6-32k"


def test_plan_is_complete_deterministic_and_shuffled():
    sp = specs(FIZZ, SQRT)
    a = runner.build_plan([CFG, CFG2], sp, 3, 7, ROOT / "tasks")
    b = runner.build_plan([CFG, CFG2], sp, 3, 7, ROOT / "tasks")
    c = runner.build_plan([CFG, CFG2], sp, 3, 8, ROOT / "tasks")
    ids = [t["trial_id"] for t in a["order"]]
    assert len(ids) == len(set(ids)) == 2 * 2 * 3
    assert ids == [t["trial_id"] for t in b["order"]]
    assert ids != [t["trial_id"] for t in c["order"]]
    assert sorted(ids) != ids  # shuffled, not in sorted/grouped order


def test_per_task_trials_override_n_and_scale_parity():
    sp = [replace(s, trials=2) if s.id == SQRT else s for s in specs(FIZZ, SQRT)]
    plan = runner.build_plan([CFG], sp, 5, 7, ROOT / "tasks", parity_fraction=0.4)
    count = {}
    for t in plan["order"]:
        key = (t["task"], t["instrumented"])
        count[key] = count.get(key, 0) + 1
    assert count == {(FIZZ, True): 5, (FIZZ, False): 2, (SQRT, True): 2, (SQRT, False): 1}
    assert plan["trials_per_task"] == {FIZZ: 5, SQRT: 2}
    uniform = runner.build_plan([CFG], sp, 5, 7, ROOT / "tasks", use_task_trials=False)
    assert uniform["trials_per_task"] == {FIZZ: 5, SQRT: 5} and len(uniform["order"]) == 10


def test_changing_trials_per_task_is_a_plan_mismatch(tmp_path):
    sp = specs(FIZZ, SQRT)
    runner.load_or_create_plan(tmp_path, runner.build_plan([CFG], sp, 3, 1, ROOT / "tasks"))
    other = [replace(s, trials=1) if s.id == SQRT else s for s in sp]
    with pytest.raises(runner.PlanMismatchError):
        runner.load_or_create_plan(tmp_path, runner.build_plan([CFG], other, 3, 1, ROOT / "tasks"))


def test_old_plans_without_trials_per_task_still_match(tmp_path):
    sp = specs(FIZZ)
    plan = runner.build_plan([CFG], sp, 3, 1, ROOT / "tasks")
    old = {k: v for k, v in plan.items() if k != "trials_per_task"}
    (tmp_path / "plan.json").write_text(json.dumps(old))
    assert runner.load_or_create_plan(tmp_path, plan) == old


def test_run_all_records_the_previous_trial_and_status_by_task(tmp_path):
    sp = specs(FIZZ, SQRT)

    def fake_attempt(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics, instrumented=True):
        return {"status": "pass", "attempt": attempt, "elapsed_s": 1.0}

    plan = runner.run_all([CFG], sp, 2, 3, tmp_path / "out", tmp_path / "s", tmp_path / "m.jsonl",
                          attempt_fn=fake_attempt, log=lambda *_: None)
    order = [t["trial_id"] for t in plan["order"]]
    recs = [json.loads((tmp_path / "out/trials" / tid / "record.json").read_text()) for tid in order]
    assert recs[0]["prev_trial_id"] is None
    assert [r["prev_trial_id"] for r in recs[1:]] == order[:-1]
    assert [r["prev_task"] for r in recs[1:]] == [t["task"] for t in plan["order"][:-1]]
    table = runner.status_table(tmp_path / "out", by_task=True)
    assert "4/4 trials done" in table and f"{FIZZ}: 2/2" in table and f"{SQRT}: 2/2" in table


def test_trial_seed_deterministic_and_distinct():
    assert runner.trial_seed(1, "a") == runner.trial_seed(1, "a")
    assert runner.trial_seed(1, "a") != runner.trial_seed(1, "b")
    assert runner.trial_seed(1, "a") != runner.trial_seed(2, "a")
    assert 0 <= runner.trial_seed(1, "a") < 2**31


def test_plan_reuse_mismatch_and_force(tmp_path):
    sp = specs(FIZZ)
    p1 = runner.build_plan([CFG], sp, 2, 1, ROOT / "tasks")
    assert runner.load_or_create_plan(tmp_path, p1) == p1
    again = runner.build_plan([CFG], sp, 2, 1, ROOT / "tasks")
    assert runner.load_or_create_plan(tmp_path, again)["order"] == p1["order"]
    other = runner.build_plan([CFG], sp, 3, 1, ROOT / "tasks")
    with pytest.raises(runner.PlanMismatchError):
        runner.load_or_create_plan(tmp_path, other)
    assert runner.load_or_create_plan(tmp_path, other, force=True) == other


# ---- classification ----------------------------------------------------------------------------

def verdict(passed=True, detail="ok"):
    return TaskVerdict("t", passed, [VerifierResult("rubric", passed, detail)])


def ev(steps=1, tools=("read",), text="x"):
    return {"steps": steps, "tools": list(tools), "final_text": text}


def run_info(timed_out=False):
    return {"timed_out": timed_out}


def test_classify_cases():
    c = runner.classify
    assert c(run_info(), ev(), verdict(True), []) == ("pass", None)
    assert c(run_info(), ev(), verdict(False), []) == ("fail", None)
    assert c(run_info(True), ev(), verdict(False), []) == ("timeout", None)
    assert c(run_info(), ev(0, (), ""), verdict(False), []) == ("harness_error", "no_events")
    assert c(run_info(True), ev(0, (), ""), verdict(False), [])[1] == "timeout_without_events"
    assert c(run_info(), ev(), verdict(False, "verifier error: X"), [])[1] == "verifier_crash"
    assert c(run_info(), ev(), verdict(True), [500])[0] == "harness_error"


def test_proxy_errors_filters_trial_attempt_and_ignores_aborts(tmp_path):
    m = tmp_path / "m.jsonl"
    rows = [
        {"trial_id": "t", "attempt": "1", "status": 500},
        {"trial_id": "t", "attempt": "2", "status": 200},
        {"trial_id": "t", "attempt": "2", "status": 200, "aborted": True},
        {"trial_id": "u", "attempt": "1", "status": 502},
    ]
    m.write_text("\n".join(json.dumps(r) for r in rows) + "\nnot json\n")
    assert runner.proxy_errors(m, "t", 1) == [500]
    assert runner.proxy_errors(m, "t", 2) == []
    assert runner.proxy_errors(tmp_path / "none", "t", 1) == []


# ---- run_trial / run_all with a stub attempt function ------------------------------------------

def stub(statuses, calls):
    it = iter(statuses)

    def fn(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics, instrumented=True):
        calls.append((tid, attempt, seed))
        st = next(it)
        return {"status": st, "reason": "boom" if st == "harness_error" else None,
                "attempt": attempt, "started_utc": "t", "elapsed_s": 1.0}
    return fn


def item(sp, cfg=CFG, k=0):
    return {"trial_id": runner.trial_id(cfg.id, sp.id, k), "config": cfg.id, "task": sp.id, "k": k}


def test_wrong_answer_is_never_retried(tmp_path):
    sp = specs(FIZZ)[0]
    calls = []
    rec = runner.run_trial(sp, CFG, item(sp), 1, tmp_path, tmp_path, tmp_path / "m", 3,
                           stub(["fail"], calls))
    assert rec["status"] == "fail" and len(calls) == 1 and len(rec["attempts"]) == 1


def test_harness_error_is_retried_then_succeeds(tmp_path):
    sp = specs(FIZZ)[0]
    calls = []
    rec = runner.run_trial(sp, CFG, item(sp), 1, tmp_path, tmp_path, tmp_path / "m", 2,
                           stub(["harness_error", "harness_error", "pass"], calls))
    assert rec["status"] == "pass" and len(calls) == 3
    assert [a["status"] for a in rec["attempts"]] == ["harness_error", "harness_error", "pass"]
    assert len({c[2] for c in calls}) == 1  # same seed across attempts


def test_harness_failed_after_retries_exhausted(tmp_path):
    sp = specs(FIZZ)[0]
    calls = []
    rec = runner.run_trial(sp, CFG, item(sp), 1, tmp_path, tmp_path, tmp_path / "m", 1,
                           stub(["harness_error", "harness_error"], calls))
    assert rec["status"] == "harness_failed" and len(calls) == 2 and rec["reason"] == "boom"


def test_seedless_config_passes_no_seed(tmp_path):
    sp = specs(FIZZ)[0]
    calls = []
    runner.run_trial(sp, CFG2, item(sp, CFG2), 1, tmp_path, tmp_path, tmp_path / "m", 0,
                     stub(["pass"], calls))
    assert calls[0][2] is None


def test_run_all_resumes_and_limit(tmp_path):
    sp = specs(FIZZ, SQRT)
    calls = []
    fn = stub(["pass"] * 20, calls)
    kw = dict(configs=[CFG], specs=sp, n=2, run_seed=3, out=tmp_path / "out", scratch=tmp_path,
              metrics=tmp_path / "m", attempt_fn=fn, log=lambda *_: None)
    plan = runner.run_all(limit=2, **kw)
    assert len(calls) == 2
    assert runner.status_table(tmp_path / "out").startswith("2/4 trials done")
    runner.run_all(**kw)  # resume: only the remaining 2 run
    assert len(calls) == 4 and len({c[0] for c in calls}) == 4
    runner.run_all(**kw)  # nothing left
    assert len(calls) == 4
    assert [c[0] for c in calls] == [t["trial_id"] for t in plan["order"]]  # randomised order kept
    assert not list((tmp_path / "out").rglob("*.tmp"))


def test_unfinished_trial_without_record_is_rerun(tmp_path):
    sp = specs(FIZZ)
    out = tmp_path / "out"
    (out / "trials" / "cfg__coding_02_fizzbuzz_gen__00").mkdir(parents=True)  # crashed mid-trial
    calls = []
    runner.run_all([CFG], sp, 1, 1, out, tmp_path, tmp_path / "m", attempt_fn=stub(["pass"], calls),
                   log=lambda *_: None)
    assert len(calls) == 1


# ---- end to end with a fake `opencode` executable ----------------------------------------------

FAKE = '''#!{py}
import json, os, pathlib, sys, time
args = sys.argv[1:]
work = args[args.index("--dir") + 1]
mode = os.environ["FAKE_OC_MODE"]
state = pathlib.Path(os.environ["FAKE_OC_STATE"])
n = int(state.read_text()) if state.exists() else 0
state.write_text(str(n + 1))
pathlib.Path(str(state) + ".env").write_text(os.environ.get("OPENCODE_DISABLE_CLAUDE_CODE", "unset"))
pathlib.Path(str(state) + ".tmpdir").write_text(os.environ.get("TMPDIR", "unset"))
def ev(t, **part):
    print(json.dumps({{"type": t, "sessionID": "ses_fake", "part": part}}), flush=True)
if mode == "crash" or (mode == "crash_then_ok" and n == 0):
    sys.exit(1)
if mode == "hang":
    time.sleep(60)
ev("step_start")
if mode in ("ok", "crash_then_ok"):
    pathlib.Path(work, "fizzbuzz.c").write_text(open(os.environ["FAKE_OC_GOOD"]).read())
ev("tool_use", tool="write", state={{"status": "completed"}})
ev("step_finish", tokens={{"input": 10, "output": 5}}, reason="tool-calls")
if mode == "hang_after_step":
    time.sleep(60)
ev("text", text="done")
ev("step_finish", tokens={{"input": 12, "output": 3}}, reason="stop")
'''


@pytest.fixture
def fake_oc(tmp_path, monkeypatch):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    exe = bin_dir / "opencode"
    exe.write_text(FAKE.format(py=sys.executable))
    exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    monkeypatch.setenv("PATH", f"{bin_dir}{os.pathsep}{os.environ['PATH']}")
    monkeypatch.setenv("FAKE_OC_STATE", str(tmp_path / "calls"))
    monkeypatch.setenv("FAKE_OC_GOOD", str(GOLDEN / FIZZ / "good/fizzbuzz.c"))

    def go(mode, max_retries=0, timeout_s=None):
        monkeypatch.setenv("FAKE_OC_MODE", mode)
        sp = specs(FIZZ)[0]
        if timeout_s:
            sp = replace(sp, timeout_s=timeout_s)
        return runner.run_trial(sp, CFG, item(sp), 5, tmp_path / "out", tmp_path / "scratch",
                                tmp_path / "metrics.jsonl", max_retries)
    return go


def test_e2e_pass_records_everything(fake_oc, tmp_path):
    rec = fake_oc("ok")
    assert rec["status"] == "pass" and rec["verdict_passed"] is True
    assert rec["steps"] == 2 and rec["tool_sequence"] == ["write"] and rec["final_answer"] == "done"
    assert rec["seed"] == runner.trial_seed(5, rec["trial_id"]) and rec["wall_end"] >= rec["wall_start"]
    assert (tmp_path / "out/trials" / rec["trial_id"] / "artifacts/fizzbuzz.c").exists()
    assert (tmp_path / "out/trials" / rec["trial_id"] / "events.jsonl").exists()
    assert not (tmp_path / "scratch" / rec["trial_id"]).exists()  # workdir cleaned


def test_e2e_opencode_runs_isolated_from_the_authors_claude_setup(fake_oc, tmp_path, monkeypatch):
    monkeypatch.setenv("OPENCODE_DISABLE_CLAUDE_CODE", "0")  # even a user override must not leak into trials
    fake_oc("ok")
    assert (tmp_path / "calls.env").read_text() == "1"


def test_manifest_records_isolation():
    from harness import manifest
    assert manifest.build_manifest()["opencode_isolation"] == {"OPENCODE_DISABLE_CLAUDE_CODE": "1"}


def test_e2e_wrong_answer_is_fail_not_retried(fake_oc, tmp_path):
    rec = fake_oc("wrong", max_retries=2)
    assert rec["status"] == "fail" and len(rec["attempts"]) == 1
    assert (tmp_path / "calls").read_text() == "1"


def test_e2e_crash_is_retried(fake_oc, tmp_path):
    rec = fake_oc("crash_then_ok", max_retries=2)
    assert rec["status"] == "pass" and len(rec["attempts"]) == 2
    assert rec["attempts"][0]["reason"] == "no_events"
    assert (tmp_path / "out/trials" / rec["trial_id"] / "events.attempt1.jsonl").parent.exists()


def test_e2e_persistent_crash_becomes_harness_failed(fake_oc):
    rec = fake_oc("crash", max_retries=1)
    assert rec["status"] == "harness_failed" and len(rec["attempts"]) == 2


def test_e2e_timeout_with_progress_is_an_outcome(fake_oc, tmp_path):
    rec = fake_oc("hang_after_step", max_retries=2, timeout_s=3)
    assert rec["status"] == "timeout" and len(rec["attempts"]) == 1
    assert rec["timed_out"] is True


def test_e2e_hang_without_events_is_harness_error(fake_oc):
    rec = fake_oc("hang", max_retries=0, timeout_s=2)
    assert rec["status"] == "harness_failed" and rec["reason"] == "timeout_without_events"


def test_config_options_and_seed_reach_opencode_config(tmp_path):
    from harness import calibrate
    sp = specs(FIZZ)[0]
    cfg = calibrate.build_config(sp, "ollama/m", None, "t", 2, {"seed": 9, "temperature": 0.0})
    assert cfg["provider"]["ollama"]["models"]["m"]["options"] == {"seed": 9, "temperature": 0.0}
    assert cfg["provider"]["ollama"]["options"]["headers"]["X-Attempt"] == "2"


def test_stalled_trial_is_a_retried_harness_failure_not_an_outcome(fake_oc, monkeypatch):
    monkeypatch.setattr(runner.calibrate, "POLL_S", 0.2)
    monkeypatch.setattr(runner, "STALL_S", 1.0)
    monkeypatch.setattr(runner.stall, "make_probe", lambda: (lambda pid: False))
    rec = fake_oc("hang_after_step", max_retries=1)  # one step, then silence
    assert rec["status"] == "harness_failed" and rec["reason"] == "stalled"
    assert len(rec["attempts"]) == 2 and all(a["reason"] == "stalled" for a in rec["attempts"])


def run_cli(monkeypatch, capsys, *args):
    monkeypatch.setattr(sys, "argv", ["runner", "run", "--configs", str(ROOT / "configs/baseline_32k.yaml"),
                                      "--out", "/nonexistent/out", "--scratch", "/nonexistent/s", "--dry-run", *args])
    code = runner.main()
    return code, capsys.readouterr()


def test_cli_suite_selection_and_task_trials(monkeypatch, capsys):
    from harness.taskspec import load_all
    all_specs = load_all(ROOT / "tasks")
    n_core = sum("core" in s.suite for s in all_specs)
    code, cap = run_cli(monkeypatch, capsys, "--suite", "core", "--n", "2")
    assert code == 0 and cap.out.rstrip().endswith(f"{2 * n_core} trials")
    assert "bugfix_01_moving_average" not in cap.out  # guard task not in core
    code, cap = run_cli(monkeypatch, capsys, "--n", "10")  # guards honour their own n = 3
    assert "bugfix_01_moving_average=3" in cap.out and "c_ringbuf_03_ringbuffer=10" in cap.out
    code, cap = run_cli(monkeypatch, capsys, "--n", "10", "--ignore-task-trials")
    assert cap.out.rstrip().endswith(f"{10 * len(all_specs)} trials")
    code, cap = run_cli(monkeypatch, capsys, "--suite", "core", "--tasks", "coding_02_fizzbuzz_gen", "--n", "4")
    assert cap.out.rstrip().endswith("4 trials")


def test_cli_unknown_suite_and_empty_selection(monkeypatch, capsys):
    code, cap = run_cli(monkeypatch, capsys, "--suite", "nope")
    assert code == 2 and "unknown suite" in cap.err and "core" in cap.err
    code, cap = run_cli(monkeypatch, capsys, "--suite", "guard", "--tasks", "coding_02_fizzbuzz_gen")
    assert code == 2 and "no tasks selected" in cap.err


def test_n_for_overrides_n_and_task_trials(tmp_path):
    sp = specs(FIZZ, SQRT)
    plan = runner.build_plan([CFG], sp, 5, 7, ROOT / "tasks", n_for={SQRT: 2})
    assert plan["trials_per_task"] == {FIZZ: 5, SQRT: 2} and len(plan["order"]) == 7
    guard = [replace(s, trials=3) for s in sp]
    plan = runner.build_plan([CFG], guard, 5, 7, ROOT / "tasks", n_for={FIZZ: 4})
    assert plan["trials_per_task"] == {FIZZ: 4, SQRT: 3}  # n_for beats the task's own trials


def test_cli_n_for(monkeypatch, capsys):
    code, cap = run_cli(monkeypatch, capsys, "--tasks", FIZZ, SQRT, "--n", "4", "--n-for", f"{SQRT}=1")
    assert code == 0 and cap.out.rstrip().endswith("5 trials") and f"{SQRT}=1" in cap.out
    code, cap = run_cli(monkeypatch, capsys, "--tasks", FIZZ, "--n-for", f"{SQRT}=1")
    assert code == 2 and "--n-for" in cap.err
    code, cap = run_cli(monkeypatch, capsys, "--tasks", FIZZ, "--n-for", f"{FIZZ}=zero")
    assert code == 2


def test_every_trial_gets_the_same_temp_dir_so_prompts_share_their_prefix(fake_oc, tmp_path):
    """OpenCode prints $TMPDIR in its first tool's description: a per-trial value would break the prefix cache."""
    fake_oc("ok")
    first = (tmp_path / "calls.tmpdir").read_text()
    assert first == str(tmp_path / "scratch" / "shared_tmp")
    fake_oc("ok")
    assert (tmp_path / "calls.tmpdir").read_text() == first and "fizzbuzz" not in first and "__" not in first
