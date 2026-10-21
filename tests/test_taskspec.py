import copy
from pathlib import Path

import pytest
import yaml

from harness import taskspec

ROOT = Path(__file__).resolve().parent.parent
TASKS = ROOT / "tasks"

GOOD = {
    "id": "demo_task",
    "title": "Demo",
    "category": "code_gen",
    "prompts": ["do it"],
    "timeout_s": 60,
    "verify": [{"type": "command", "cmd": "true"}],
}


def mutate(**kw):
    d = copy.deepcopy(GOOD)
    for k, v in kw.items():
        if v is taskspec.TaskSpecError:  # sentinel: delete key
            d.pop(k, None)
        else:
            d[k] = v
    return d


DEL = taskspec.TaskSpecError

NEGATIVE = {
    "trials_zero": (mutate(trials=0), "trials: must be an integer"),
    "trials_string": (mutate(trials="3"), "trials: must be an integer"),
    "trials_bool": (mutate(trials=True), "trials: must be an integer"),
    "suite_bad_name": (mutate(suite=["Core Suite"]), "suite:"),
    "suite_not_list": (mutate(suite="core"), "suite: must be a list"),
    "missing_verify": (mutate(verify=DEL), "missing required key 'verify'"),
    "empty_verify": (mutate(verify=[]), "verify: must be a non-empty list"),
    "bad_timeout_low": (mutate(timeout_s=1), "timeout_s"),
    "bad_timeout_str": (mutate(timeout_s="60"), "timeout_s"),
    "bad_timeout_bool": (mutate(timeout_s=True), "timeout_s"),
    "unknown_top_key": (mutate(verifier="x"), "unknown key 'verifier'"),
    "bad_id": (mutate(id="Bad-Id"), "id: must match"),
    "bad_category": (mutate(category="nonsense"), "category"),
    "empty_prompts": (mutate(prompts=[]), "prompts"),
    "blank_prompt": (mutate(prompts=["  "]), "prompts"),
    "unknown_tool": (mutate(tools=["rm_rf"]), "unknown tool 'rm_rf'"),
    "bad_fixture": (mutate(fixture="../etc"), "fixture"),
    "unknown_verifier": (mutate(verify=[{"type": "magic"}]), "unknown"),
    "verifier_missing_key": (mutate(verify=[{"type": "compile_run"}]), "requires 'sources'"),
    "verifier_extra_key": (mutate(verify=[{"type": "command", "cmd": "x", "foo": 1}]), "unknown key 'foo'"),
    "bad_callable": (mutate(verify=[{"type": "python", "callable": "no_colon"}]), "callable"),
    "empty_rubric": (mutate(verify=[{"type": "rubric"}]), "rubric needs at least one"),
    "rubric_bad_regex": (
        mutate(verify=[{"type": "rubric", "required": [{"name": "x", "any_of": ["("]}]}]), "valid regexes"),
    "rubric_min_gt_max": (mutate(verify=[{"type": "rubric", "min_words": 9, "max_words": 3}]), "min_words > max_words"),
    "bad_setup": (mutate(setup="echo hi"), "setup"),
    "py_test_missing_source": (mutate(verify=[{"type": "py_test"}]), "requires 'test_source'"),
    "bad_env": (mutate(env={"A": 1}), "env"),
    "not_a_mapping": (["a"], "top level must be a mapping"),
}


def test_good_spec_is_valid():
    assert taskspec.validate_spec(GOOD, filename_stem="demo_task") == []


def test_negative_case_count():
    assert len(NEGATIVE) >= 12


@pytest.mark.parametrize("name", sorted(NEGATIVE))
def test_negative_cases_rejected_with_clear_message(name):
    data, fragment = NEGATIVE[name]
    errs = taskspec.validate_spec(data)
    assert errs, f"{name} was accepted"
    assert any(fragment in e for e in errs), errs


def test_filename_must_match_id():
    errs = taskspec.validate_spec(GOOD, filename_stem="other")
    assert any("does not match file name" in e for e in errs)


def test_all_errors_reported_together():
    errs = taskspec.validate_spec({"id": "x"})
    assert len(errs) >= 5


def test_load_task_raises_with_source(tmp_path):
    p = tmp_path / "demo_task.yaml"
    p.write_text(yaml.safe_dump(mutate(timeout_s=1)))
    with pytest.raises(taskspec.TaskSpecError) as ei:
        taskspec.load_task(p)
    assert "demo_task.yaml" in str(ei.value)


def test_load_task_invalid_yaml(tmp_path):
    p = tmp_path / "x.yaml"
    p.write_text("a: [unclosed")
    with pytest.raises(taskspec.TaskSpecError, match="invalid YAML"):
        taskspec.load_task(p)


def test_check_assets_reports_missing(tmp_path):
    spec = taskspec.parse_spec(mutate(
        fixture="nofix", verify=[{"type": "c_test", "sources": ["a.c"], "test_source": "t.c"}]))
    errs = taskspec.check_assets(spec, tmp_path, tmp_path)
    assert len(errs) == 2


def test_committed_tasks_are_valid_and_complete():
    specs = taskspec.load_all(TASKS)
    ids = {s.id for s in specs}
    assert len(specs) == 14
    assert {s.category for s in specs} >= {
        "bugfix", "refactor", "log_triage", "repo_qa", "long_context", "code_gen"}
    assert {"coding_01_my_sqrt_review", "comms_07_confirm_send", "research_08e_free_will"} <= ids
    for s in specs:
        assert taskspec.check_assets(s, ROOT / "fixtures/tasks", ROOT / "fixtures/verify") == []


def test_main_cli_flags_bad_task(tmp_path, monkeypatch, capsys):
    (tmp_path / "demo_task.yaml").write_text(yaml.safe_dump(mutate(timeout_s=1)))
    monkeypatch.setattr("sys.argv", ["taskspec", "--tasks", str(tmp_path)])
    assert taskspec.main() == 1
    assert "FAIL demo_task.yaml" in capsys.readouterr().out


def test_main_cli_ok_on_repo(monkeypatch, capsys):
    monkeypatch.chdir(ROOT)
    monkeypatch.setattr("sys.argv", ["taskspec"])
    assert taskspec.main() == 0


def test_trials_and_suite_are_accepted_and_parsed():
    spec = taskspec.parse_spec(mutate(trials=3, suite=["core", "guard_2"]))
    assert not taskspec.validate_spec(mutate(trials=3, suite=["core", "guard_2"]))
    assert spec.trials == 3 and spec.suite == ["core", "guard_2"]
    assert taskspec.parse_spec(GOOD).trials is None and taskspec.parse_spec(GOOD).suite == []


def test_repo_suites_partition_the_tasks():
    specs = taskspec.load_all(TASKS)
    core = {s.id for s in specs if "core" in s.suite}
    guard = {s.id for s in specs if "guard" in s.suite}
    assert core and guard and not core & guard and core | guard == {s.id for s in specs}
    assert all(s.trials == 3 for s in specs if s.id in guard) and all(s.trials is None for s in specs if s.id in core)
