import hashlib
import json
import shutil
from pathlib import Path

import pytest

from harness import replay, runner

ROOT = Path(__file__).resolve().parent.parent
FIXTURES = ROOT / "fixtures/replay"
NAMES = ["parity_pilot", "traced_pilot"]


def tree_hash(path: Path) -> str:
    h = hashlib.sha256()
    for p in sorted(path.rglob("*")):
        if p.is_file():
            h.update(str(p.relative_to(path)).encode())
            h.update(p.read_bytes())
    return h.hexdigest()


@pytest.mark.parametrize("name", NAMES)
def test_replay_regenerates_the_committed_golden_byte_for_byte(name, tmp_path):
    out = tmp_path / "out"
    res = replay.replay(FIXTURES / name, out)
    expected = FIXTURES / name / "expected"
    for f in sorted(expected.iterdir()):
        assert (out / f.name).read_bytes() == f.read_bytes(), f"{f.name} differs from the golden copy"
    assert {p.name for p in out.iterdir()} == {p.name for p in expected.iterdir()}
    assert (out / "SHA256SUMS").read_text() == (expected / "SHA256SUMS").read_text()
    assert res["replay"]["mismatches"] == [] and res["replay"]["skipped"] == {}


@pytest.mark.parametrize("name", NAMES)
def test_golden_hashes_match_the_files(name):
    expected = FIXTURES / name / "expected"
    for line in (expected / "SHA256SUMS").read_text().splitlines():
        digest, fname = line.split("  ")
        assert hashlib.sha256((expected / fname).read_bytes()).hexdigest() == digest


def test_replay_never_modifies_the_recorded_run(tmp_path):
    before = tree_hash(FIXTURES / "traced_pilot" / "trials")
    replay.replay(FIXTURES / "traced_pilot", tmp_path / "o")
    assert tree_hash(FIXTURES / "traced_pilot" / "trials") == before
    assert not list((FIXTURES / "traced_pilot").rglob("trace.json"))


def test_all_recorded_verdicts_of_the_parity_pilot_are_reproduced(tmp_path):
    res = replay.replay(FIXTURES / "parity_pilot", tmp_path / "o")["replay"]
    assert res["trials"] == res["verified"] == 30
    assert all(v["recorded"] == v["replayed"] for v in res["verdicts"].values())
    assert any(not v["recorded"] for v in res["verdicts"].values())  # failing verdicts reproduce too


def test_traced_replay_includes_join_decomposition_and_gpu(tmp_path):
    s = replay.replay(FIXTURES / "traced_pilot", tmp_path / "o")["summary"]
    d = s["decomposition"]["baseline_32k"]
    assert d["all"]["n"] == 2 and abs(d["all"]["pct"]["unattributed"]) < 5.0 and d["gpu_duty_pct"] > 20
    assert (tmp_path / "o" / "report.html").read_text().count("<svg") >= 3


def copy_run(name, tmp_path):
    dst = tmp_path / "run"
    shutil.copytree(FIXTURES / name, dst, ignore=shutil.ignore_patterns("expected"))
    return dst


def test_a_tampered_verdict_is_detected(tmp_path):
    run = copy_run("parity_pilot", tmp_path)
    p = next(run.glob("trials/*__00/record.json"))
    rec = json.loads(p.read_text())
    rec["verdict_passed"] = not rec["verdict_passed"]
    p.write_text(json.dumps(rec))
    res = replay.replay(run, tmp_path / "o")["replay"]
    assert res["mismatches"] == [rec["trial_id"]]


def test_a_tampered_final_answer_changes_the_replayed_verdict(tmp_path):
    run = copy_run("parity_pilot", tmp_path)
    p = next(run.glob("trials/*coding_06*__00/record.json"))
    rec = json.loads(p.read_text())
    rec["final_answer"] = "word " * 300  # violates the 150-word limit, but the record says it passed
    rec["verdict_passed"] = True
    p.write_text(json.dumps(rec))
    res = replay.replay(run, tmp_path / "o")["replay"]
    assert rec["trial_id"] in res["mismatches"]


def test_cli_exit_codes(tmp_path, monkeypatch, capsys):
    monkeypatch.setattr("sys.argv", ["replay", "--run", str(FIXTURES / "traced_pilot"), "--out", str(tmp_path / "ok")])
    assert replay.main() == 0 and "0 mismatches" in capsys.readouterr().out
    run = copy_run("parity_pilot", tmp_path)
    p = next(run.glob("trials/*__00/record.json"))
    rec = json.loads(p.read_text())
    rec["verdict_passed"] = not rec["verdict_passed"]
    p.write_text(json.dumps(rec))
    monkeypatch.setattr("sys.argv", ["replay", "--run", str(run), "--out", str(tmp_path / "bad")])
    assert replay.main() == 1 and "MISMATCH" in capsys.readouterr().out


def test_skips_are_reported_not_hidden(tmp_path):
    run = copy_run("traced_pilot", tmp_path)
    art = next(run.glob("trials/*bugfix*/artifacts"))
    shutil.rmtree(art)  # the recorded artifacts are gone: cannot verify, must say so
    res = replay.replay(run, tmp_path / "o")["replay"]
    key = next(k for k in res["skipped"] if "bugfix" in k)
    assert "artifacts not recorded" in res["skipped"][key] and res["mismatches"] == []
    failed = next(run.glob("trials/*coding_05*/record.json"))
    rec = json.loads(failed.read_text())
    rec.pop("verdict_passed")
    failed.write_text(json.dumps(rec))
    res = replay.replay(run, tmp_path / "o2")["replay"]
    assert any("no recorded verdict" in v for v in res["skipped"].values())


def test_no_verify_flag_skips_verifier_replay(tmp_path):
    res = replay.replay(FIXTURES / "parity_pilot", tmp_path / "o", verify=False)["replay"]
    assert res["trials"] == 0 and res["verified"] == 0


def test_runner_keeps_comm_state_for_replay(tmp_path):
    comm = tmp_path / "comm"
    comm.mkdir()
    (comm / "email_store.json").write_text("[1]")
    (comm / "comm_tools_mcp.py").write_text("# not copied")
    trial = tmp_path / "trial"
    runner.save_comm_state(comm, trial)
    assert [p.name for p in (trial / "artifacts" / "comm_state").iterdir()] == ["email_store.json"]
    runner.save_comm_state(None, tmp_path / "other")  # tasks without comm-tools: nothing to do
    assert not (tmp_path / "other").exists()


def test_comms_trial_without_recorded_state_is_skipped(tmp_path):
    from harness.taskspec import load_task
    spec = load_task(ROOT / "tasks/comms_07_confirm_send.yaml")
    trial = tmp_path / "t"
    trial.mkdir()
    out = replay.replay_verdict(spec, trial, {"verdict_passed": True, "files": []}, ROOT / "fixtures/tasks",
                                ROOT / "fixtures/verify")
    assert "comm-tools state" in out["skipped"]
    (trial / "artifacts" / "comm_state").mkdir(parents=True)
    (trial / "artifacts" / "comm_state" / "email_store.json").write_text(
        (ROOT / "fixtures/golden/comms_07_confirm_send/good/email_store.json").read_text())
    (trial / "artifacts" / "comm_state" / "calendar_store.json").write_text(
        (ROOT / "fixtures/golden/comms_07_confirm_send/good/calendar_store.json").read_text())
    out = replay.replay_verdict(spec, trial, {"verdict_passed": True, "files": []}, ROOT / "fixtures/tasks",
                                ROOT / "fixtures/verify")
    assert out == {"recorded": True, "replayed": True, "match": True}
