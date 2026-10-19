"""
harness/sandbox.py

Run verifier commands in a throw-away working directory with resource limits.

Limits (POSIX rlimits, applied in the child): CPU seconds, address space, max file size.
Network isolation uses ``unshare -rn`` when the kernel allows unprivileged user namespaces;
otherwise the run proceeds without it and ``RunResult.network_isolated`` is False so that
callers/reports can say so. (Not a security boundary - the code under test is model output
on a developer box, the goal is to stop runaway loops, huge files and stray network use
in the common case.)
"""

import os
import resource
import shutil
import signal
import subprocess
import tempfile
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass
from functools import cache
from pathlib import Path

DEFAULT_TIMEOUT_S = 60
MEM_LIMIT_BYTES = 4 * 1024**3
FILE_LIMIT_BYTES = 100 * 1024**2


@dataclass
class RunResult:
    returncode: int
    stdout: str
    stderr: str
    timed_out: bool = False
    network_isolated: bool = False


@cache
def can_unshare_net() -> bool:
    if not shutil.which("unshare"):
        return False
    try:
        return subprocess.run(["unshare", "-rn", "true"], capture_output=True,
                              timeout=5).returncode == 0
    except (OSError, subprocess.SubprocessError):
        return False


def _limits(cpu_s: int, mem_limit: int | None):
    def apply() -> None:
        resource.setrlimit(resource.RLIMIT_CPU, (cpu_s, cpu_s + 1))
        if mem_limit:
            resource.setrlimit(resource.RLIMIT_AS, (mem_limit, mem_limit))
        resource.setrlimit(resource.RLIMIT_FSIZE, (FILE_LIMIT_BYTES, FILE_LIMIT_BYTES))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    return apply


def run(argv: list[str], cwd: Path, timeout_s: int = DEFAULT_TIMEOUT_S,
        env: dict[str, str] | None = None,
        mem_limit: int | None = MEM_LIMIT_BYTES) -> RunResult:
    """Run argv in cwd; kills the whole process group on timeout.

    mem_limit=None disables the address-space cap (AddressSanitizer maps terabytes of shadow)."""
    full_env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "HOME": str(cwd),
                "LANG": "C.UTF-8", "PYTHONDONTWRITEBYTECODE": "1"}
    full_env.update(env or {})
    isolated = can_unshare_net()
    if isolated:
        argv = ["unshare", "-rn", *argv]
    proc = subprocess.Popen(argv, cwd=cwd, env=full_env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, errors="replace",
                            start_new_session=True, preexec_fn=_limits(timeout_s + 5, mem_limit))  # noqa: PLW1509
    try:
        out, err = proc.communicate(timeout=timeout_s)
        return RunResult(proc.returncode, out, err, False, isolated)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        out, err = proc.communicate()
        return RunResult(-9, out or "", err or "", True, isolated)


@contextmanager
def scratch_copy(src: Path) -> Iterator[Path]:
    """Copy src to a temp dir that is deleted afterwards (verifiers never touch the original)."""
    with tempfile.TemporaryDirectory(prefix="agent_opt_verify_") as d:
        dest = Path(d) / "work"
        shutil.copytree(src, dest, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        yield dest
