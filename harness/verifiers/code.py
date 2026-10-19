"""Verifiers that compile/run code produced by the agent (C and Python)."""

import shutil
import sys
from pathlib import Path

from harness import sandbox
from harness.verifiers import VerifierResult, VerifyContext

DEFAULT_CFLAGS = ["-Wall", "-Werror"]
SANITIZE = ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined"]


def _missing(work: Path, names: list[str]) -> list[str]:
    return [n for n in names if not (work / n).is_file()]


def _compile(work: Path, cflags: list[str], files: list[str]) -> tuple[bool, str]:
    r = sandbox.run(["gcc", *cflags, "-I.", *files, "-o", "prog"], work, 120)
    return r.returncode == 0, (r.stderr.strip()[:300] or "compile failed")


def verify_compile_run(params: dict, ctx: VerifyContext) -> VerifierResult:
    with sandbox.scratch_copy(ctx.workdir) as work:
        miss = _missing(work, params["sources"])
        if miss:
            return VerifierResult("compile_run", False, f"missing {miss}")
        ok, msg = _compile(work, params.get("cflags", DEFAULT_CFLAGS), params["sources"])
        if not ok:
            return VerifierResult("compile_run", False, msg)
        r = sandbox.run(["./prog"], work, params.get("run_timeout_s", 10))
    if r.timed_out:
        return VerifierResult("compile_run", False, "program timed out")
    failures = []
    if r.returncode != params.get("expect_exit", 0):
        failures.append(f"exit {r.returncode}")
    lines = {ln.strip() for ln in r.stdout.splitlines()}
    for want in params.get("expect_lines", []):
        if want not in lines:
            failures.append(f"output lacks line {want!r}")
    return VerifierResult("compile_run", not failures, "; ".join(failures) or "ok")


def verify_c_test(params: dict, ctx: VerifyContext) -> VerifierResult:
    """Hidden test_source #includes the agent's source file(s); exit 0 = pass."""
    hidden = ctx.verify_root / ctx.spec.id / params["test_source"]
    with sandbox.scratch_copy(ctx.workdir) as work:
        miss = _missing(work, params["sources"])
        if miss:
            return VerifierResult("c_test", False, f"missing {miss}")
        shutil.copy(hidden, work / params["test_source"])
        # hidden tests run under ASan/UBSan so silent heap overflows and UB are caught
        ok, msg = _compile(work, [*params.get("cflags", DEFAULT_CFLAGS), *SANITIZE],
                           [params["test_source"]])
        if not ok:
            return VerifierResult("c_test", False, msg)
        r = sandbox.run(["./prog"], work, params.get("run_timeout_s", 10), mem_limit=None,
                        env={"ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0"})
    if r.timed_out:
        return VerifierResult("c_test", False, "test timed out")
    return VerifierResult("c_test", r.returncode == 0,
                          "ok" if r.returncode == 0 else (r.stderr.strip()[:300] or f"exit {r.returncode}"))


def verify_py_test(params: dict, ctx: VerifyContext) -> VerifierResult:
    """Hidden stdlib-unittest file is copied next to the agent's code and run."""
    hidden = ctx.verify_root / ctx.spec.id / params["test_source"]
    with sandbox.scratch_copy(ctx.workdir) as work:
        shutil.copy(hidden, work / params["test_source"])
        module = Path(params["test_source"]).stem
        r = sandbox.run([sys.executable, "-m", "unittest", "-q", module], work,
                        params.get("run_timeout_s", 60), env={"PYTHONPATH": "."})
    if r.timed_out:
        return VerifierResult("py_test", False, "tests timed out")
    if r.returncode == 0:
        return VerifierResult("py_test", True, "ok")
    tail = r.stderr.strip().splitlines()[-3:]
    return VerifierResult("py_test", False, " | ".join(tail)[:300] or f"exit {r.returncode}")
