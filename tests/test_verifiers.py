"""Golden-good / golden-bad checks for every task verifier (F3).

fixtures/golden/<task id>/<variant>/ holds files overlaid on the prepared workdir;
``answer.txt`` (if present) is the final answer text. Variants named ``good*`` must pass,
variants named ``bad*`` must fail.
"""

import shutil
from pathlib import Path

import pytest

from harness import sandbox
from harness.taskspec import load_all
from harness.verifiers import VerifyContext, run_verifiers
from harness.workdir import prepare_workdir

ROOT = Path(__file__).resolve().parent.parent
FIX, VERIFY, GOLDEN = ROOT / "fixtures/tasks", ROOT / "fixtures/verify", ROOT / "fixtures/golden"
SPECS = {s.id: s for s in load_all(ROOT / "tasks")}


def cases():
    out = []
    for task_dir in sorted(GOLDEN.iterdir()):
        for variant in sorted(p for p in task_dir.iterdir() if p.is_dir()):
            out.append(pytest.param(task_dir.name, variant.name, id=f"{task_dir.name}/{variant.name}"))
    return out


def evaluate(task_id: str, variant: str, tmp_path: Path):
    spec = SPECS[task_id]
    work = prepare_workdir(spec, tmp_path / "work", FIX, VERIFY)
    src = GOLDEN / task_id / variant
    answer = ""
    for f in src.iterdir():
        if f.name == "answer.txt":
            answer = f.read_text()
        else:
            shutil.copy(f, work / f.name)
    ctx = VerifyContext(workdir=work, verify_root=VERIFY, spec=spec, final_answer=answer)
    return run_verifiers(spec, ctx), work


def test_every_task_has_good_and_bad_golden():
    for tid in SPECS:
        variants = [p.name for p in (GOLDEN / tid).iterdir() if p.is_dir()]
        assert any(v.startswith("good") for v in variants), f"{tid}: no good golden"
        assert any(v.startswith("bad") for v in variants), f"{tid}: no bad golden"


@pytest.mark.parametrize(("task_id", "variant"), cases())
def test_golden(task_id, variant, tmp_path):
    verdict, _ = evaluate(task_id, variant, tmp_path)
    if variant.startswith("good"):
        assert verdict.passed, verdict.summary()
    else:
        assert not verdict.passed, verdict.summary()


def test_verifiers_do_not_modify_the_workdir(tmp_path):
    verdict, work = evaluate("coding_02_fizzbuzz_gen", "good", tmp_path)
    assert verdict.passed
    assert not (work / "prog").exists()


def test_sandbox_timeout_kills_runaway(tmp_path):
    r = sandbox.run(["bash", "-c", "while :; do :; done"], tmp_path, timeout_s=1)
    assert r.timed_out and r.returncode == -9


def test_sandbox_file_size_limit(tmp_path):
    r = sandbox.run(["bash", "-c", "head -c 300000000 /dev/zero > big"], tmp_path, timeout_s=20)
    assert r.returncode != 0 or (tmp_path / "big").stat().st_size <= sandbox.FILE_LIMIT_BYTES


def test_verifier_crash_is_a_failed_task_not_an_exception(tmp_path):
    spec = SPECS["comms_07_confirm_send"]
    from dataclasses import replace
    broken = replace(spec, verify=[{"type": "python", "callable": "harness.verifiers.nope:missing"}])
    ctx = VerifyContext(workdir=tmp_path, verify_root=VERIFY, spec=broken)
    v = run_verifiers(broken, ctx)
    assert not v.passed and "cannot load" in v.results[0].detail


def test_rubric_target_file_cannot_escape_workdir(tmp_path):
    from harness.verifiers import rubric
    (tmp_path / "w").mkdir()
    (tmp_path / "secret.txt").write_text("hello")
    ctx = VerifyContext(workdir=tmp_path / "w", verify_root=VERIFY, spec=SPECS["coding_06_tax_rates_lan"])
    res = rubric.verify({"target": "../secret.txt", "min_words": 1}, ctx)
    assert not res.passed


def test_rubric_word_limits_and_forbidden(tmp_path):
    from harness.verifiers import rubric
    ctx = VerifyContext(workdir=tmp_path, verify_root=VERIFY, spec=SPECS["coding_06_tax_rates_lan"],
                        final_answer="one two three")
    assert rubric.verify({"min_words": 3, "max_words": 3}, ctx).passed
    assert not rubric.verify({"max_words": 2}, ctx).passed
    assert not rubric.verify({"forbidden": [{"name": "x", "regex": "TWO"}]}, ctx).passed
    assert not rubric.verify({"required": [{"name": "x", "any_of": ["four"]}]}, ctx).passed


def test_cli_reports_pass_and_fail(tmp_path, monkeypatch, capsys):
    from harness.verifiers import __main__ as cli
    work = prepare_workdir(SPECS["coding_02_fizzbuzz_gen"], tmp_path / "w", FIX, VERIFY)
    shutil.copy(GOLDEN / "coding_02_fizzbuzz_gen/good/fizzbuzz.c", work)
    argv = ["x", "coding_02_fizzbuzz_gen", str(work), "--tasks", str(ROOT / "tasks"),
            "--verify-root", str(VERIFY)]
    monkeypatch.setattr("sys.argv", argv)
    assert cli.main() == 0
    (work / "fizzbuzz.c").unlink()
    assert cli.main() == 1
    assert "VERDICT: FAIL" in capsys.readouterr().out
