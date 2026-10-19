"""Generic shell-command verifier (exit code and stdout substrings)."""

from harness import sandbox
from harness.verifiers import VerifierResult, VerifyContext


def verify(params: dict, ctx: VerifyContext) -> VerifierResult:
    with sandbox.scratch_copy(ctx.workdir) as work:
        r = sandbox.run(["bash", "-c", params["cmd"]], work, params.get("run_timeout_s", 30))
    if r.timed_out:
        return VerifierResult("command", False, "timed out")
    failures = []
    want = params.get("expect_exit", 0)
    if r.returncode != want:
        failures.append(f"exit {r.returncode} != {want}: {r.stderr.strip()[:200]}")
    for s in params.get("stdout_contains", []):
        if s not in r.stdout:
            failures.append(f"stdout lacks {s!r}")
    return VerifierResult("command", not failures, "; ".join(failures) or "ok")
