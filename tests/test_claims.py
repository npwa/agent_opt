import json

import pytest

from analysis import claims


def text(t):
    return {"kind": "text", "text": t}


def tool(name, command="", exit=None):
    return {"kind": "tool", "tool": name, "command": command, "exit": exit, "status": "completed"}


@pytest.mark.parametrize("sentence", [
    "All 5 tests pass. The bug was an off-by-one.",
    "Tests now pass (4/4 OK).",
    "I verified the output matches the spec.",
    "The program compiles cleanly with -Wall.",
    "Build succeeded; successfully compiled fizzbuzz.c",
    "The script works correctly.",
    "I tested it with three inputs.",
])
def test_claims_are_found(sentence):
    assert claims.find_claims(sentence)


@pytest.mark.parametrize("sentence", [
    "I have not verified the output.",
    "I couldn't test this because gcc is missing.",
    "You should run the tests to verify.",
    "Once the tests pass we can merge.",
    "The code is untested.",
    "Done. Created fizzbuzz.c.",
    "```\nAll tests pass\n```",
])
def test_non_claims_are_ignored(sentence):
    assert not claims.find_claims(sentence)


def test_evidence_classes():
    run_ok = tool("bash", "gcc -o a a.c && ./a", 0)
    run_bad = tool("bash", "python3 -m unittest", 1)
    edit = tool("edit")
    assert claims.analyse_trial([tool("write"), text("All tests pass.")])["final_claim"] == "unverified"
    assert claims.analyse_trial([tool("write"), run_ok, text("All tests pass.")])["final_claim"] == "supported"
    assert claims.analyse_trial([tool("write"), run_bad, text("All tests pass.")])["final_claim"] == "contradicted"
    assert claims.analyse_trial([tool("write"), run_ok, edit, text("All tests pass.")])["final_claim"] == "stale"
    # a fresh passing run after the failure and the edit supports the claim again
    assert claims.analyse_trial([run_bad, edit, run_ok, text("Verified.")])["final_claim"] == "supported"
    # grep / cat are not executions
    assert claims.analyse_trial([tool("bash", "grep -n foo a.c", 0), text("Verified.")])["final_claim"] == "unverified"
    assert claims.analyse_trial([tool("write"), text("Done.")])["final_claim"] is None


def test_only_the_final_text_decides_the_trial_class():
    a = claims.analyse_trial([text("Tests pass."), tool("bash", "pytest", 0), text("Done.")])
    assert len(a["claims"]) == 1 and a["claims"][0]["evidence"] == "unverified" and not a["claims"][0]["final"]
    assert a["final_claim"] is None


def write_run(tmp_path, trials):
    for tid, task, steps, passed in trials:
        d = tmp_path / "trials" / tid
        d.mkdir(parents=True)
        (d / "record.json").write_text(json.dumps({"trial_id": tid, "task": task, "instrumented": True,
                                                   "status": "pass" if passed else "fail", "verdict_passed": passed}))
        lines = []
        for s in steps:
            if s["kind"] == "text":
                lines.append({"type": "text", "part": {"text": s["text"]}})
            else:
                lines.append({"type": "tool_use", "part": {"tool": s["tool"], "state": {
                    "input": {"command": s["command"]}, "metadata": {"exit": s["exit"]}, "status": "completed"}}})
        (d / "events.jsonl").write_text("\n".join(json.dumps(x) for x in lines))
    tasks = tmp_path / "tasks"
    tasks.mkdir()
    for t, cat in (("gen", "code_gen"), ("rev", "code_review")):
        (tasks / f"{t}.yaml").write_text(f"id: {t}\ncategory: {cat}\n")
    return tasks


def test_run_summary_and_false_claims(tmp_path):
    ok = tool("bash", "gcc a.c && ./a.out", 0)
    tasks = write_run(tmp_path, [
        ("t1", "gen", [tool("write"), ok, text("All tests pass.")], True),
        ("t2", "gen", [tool("write"), ok, text("All tests pass.")], False),   # false claim, but supported by its own run
        ("t3", "gen", [tool("write"), text("Verified it works.")], False),     # unverified and false
        ("t4", "gen", [tool("write"), text("Done.")], False),                  # no claim
        ("t5", "rev", [text("The function works correctly.")], True),          # review: not evaluated
    ])
    rows = claims.analyse_run(tmp_path, tasks)
    assert len(rows) == 5 and sum(r["applicable"] for r in rows) == 4
    s = claims.summarise(rows)
    assert s["with_final_claim"] == 3 and s["final_claim_classes"]["supported"] == 2
    assert s["final_claim_classes"]["unverified"] == 1 and s["false_claims"] == 2
    assert s["unverified_claim_and_failed"] == 1
    out = claims.write_claims(tmp_path, tasks_dir=tasks)
    assert out == s and "false claims" in (tmp_path / "claims.md").read_text()
    assert json.loads((tmp_path / "claims.json").read_text())["summary"]["applicable"] == 4
