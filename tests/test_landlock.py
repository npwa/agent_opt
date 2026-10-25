import subprocess
import sys
from pathlib import Path

import pytest

from harness import calibrate, landlock, runner

ROOT = Path(__file__).resolve().parent.parent
LANDLOCK = ROOT / "harness" / "landlock.py"
needs_landlock = pytest.mark.skipif(not landlock.supported(), reason="Landlock not available on this kernel")


def confined_python(rw, code):
    """Run `code` under Landlock with the system Python (the repo virtualenv is deliberately off limits)."""
    return subprocess.run([sys.executable, str(LANDLOCK), "--rw", str(rw), "--", "/usr/bin/python3", "-c", code],
                          capture_output=True, text=True, timeout=60)


@needs_landlock
def test_confined_process_can_use_its_directory_but_nothing_else(tmp_path):
    work, other = tmp_path / "work", tmp_path / "other"
    work.mkdir()
    other.mkdir()
    (other / "secret.txt").write_text("hidden")
    hidden_test = ROOT / "fixtures/verify/bugfix_01_moving_average/test_hidden.py"
    code = f"""
import os
def attempt(label, fn):
    try:
        fn(); print(label, "ALLOWED")
    except OSError as e:
        print(label, "denied", type(e).__name__)
attempt("write-inside", lambda: open({str(work / 'a.txt')!r}, "w").write("x"))
attempt("mkdir-inside", lambda: os.makedirs({str(work / 'd/e')!r}))
attempt("read-system", lambda: open("/usr/bin/python3", "rb").read(1))
attempt("read-other", lambda: open({str(other / 'secret.txt')!r}).read())
attempt("write-other", lambda: open({str(other / 'new.txt')!r}, "w"))
attempt("read-hidden-test", lambda: open({str(hidden_test)!r}).read())
attempt("write-repo", lambda: open({str(ROOT / 'README.md')!r}, "a"))
attempt("read-home", lambda: os.listdir({str(Path.home())!r}))
attempt("move-out", lambda: os.rename({str(work / 'a.txt')!r}, {str(other / 'a.txt')!r}))
attempt("delete-inside", lambda: os.remove({str(work / 'a.txt')!r}))
"""
    r = confined_python(work, code)
    assert r.returncode == 0, r.stderr
    out = dict(line.split(" ", 1) for line in r.stdout.splitlines())
    assert out["write-inside"] == "ALLOWED" and out["mkdir-inside"] == "ALLOWED" and out["read-system"] == "ALLOWED"
    assert out["delete-inside"] == "ALLOWED"
    for denied in ("read-other", "write-other", "read-hidden-test", "write-repo", "read-home", "move-out"):
        assert out[denied].startswith("denied"), (denied, out[denied])
    assert not (other / "a.txt").exists() and (other / "secret.txt").read_text() == "hidden"


@needs_landlock
def test_children_inherit_the_confinement(tmp_path):
    work = tmp_path / "work"
    work.mkdir()
    r = subprocess.run([sys.executable, str(LANDLOCK), "--rw", str(work), "--", "/bin/sh", "-c",
                        f"echo ok > {work}/in.txt; echo bad > {ROOT}/should_not_exist.txt; ls {Path.home()}"],
                       capture_output=True, text=True, timeout=60)
    assert (work / "in.txt").exists(), r.stderr
    assert (work / "in.txt").read_text() == "ok\n"
    assert not (ROOT / "should_not_exist.txt").exists() and "Permission denied" in r.stderr


def test_wrapper_needs_a_command_and_reports_unsupported_kernels(monkeypatch, capsys):
    with pytest.raises(SystemExit):
        landlock.main(["--rw", "/tmp"])
    monkeypatch.setattr(landlock, "abi_version", lambda: 0)
    with pytest.raises(landlock.LandlockError):
        landlock.restrict([Path("/tmp")])
    assert landlock.main(["--rw", "/tmp", "--", "true"]) == 126
    assert "not available" in capsys.readouterr().err


def test_confined_command_construction(tmp_path):
    cmd = ["opencode", "run", "x"]
    assert calibrate.confined(cmd, None) == cmd
    wrapped = calibrate.confined(cmd, [tmp_path / "a", tmp_path / "b"])
    assert wrapped[0] == sys.executable and wrapped[1].endswith("harness/landlock.py")  # a script path, not -m (cwd is the trial dir)
    assert wrapped[2:] == ["--rw", str(tmp_path / "a"), "--rw", str(tmp_path / "b"), "--", *cmd]


def test_runner_confines_by_default_and_has_an_off_switch():
    src = Path(runner.__file__).read_text()
    assert "CONFINE = True" in src and "--no-confine" in src
    assert 'env["TMPDIR"]' in src and "not d.startswith(str(ROOT))" in src  # temp files in the trial dir, venv off PATH


def test_a_missing_writable_path_is_an_error_not_a_silent_denial(tmp_path):
    with pytest.raises(landlock.LandlockError, match="does not exist"):
        landlock.restrict([tmp_path / "missing"], [])


@needs_landlock
def test_missing_default_writable_path_is_skipped(tmp_path, monkeypatch):
    """A fresh machine has no ~/.npm: that must not stop the confinement (it failed on GitHub's runner)."""
    monkeypatch.setattr(landlock, "DEFAULT_RW", [Path("/dev"), tmp_path / "no-such-dir"])
    monkeypatch.setattr(landlock, "restrict", lambda rw, ro: seen.update(rw=rw))
    seen = {}
    monkeypatch.setattr(landlock.os, "execvp", lambda *a: None)
    assert landlock.main(["--rw", str(tmp_path), "--", "true"]) is None
    assert seen["rw"] == [tmp_path, Path("/dev")]
