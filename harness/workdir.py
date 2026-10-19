"""
harness/workdir.py

Prepare the per-trial working directory for a task: copy the fixture, then run the
task's ``setup`` commands (e.g. generate a large log) with TASK_VERIFY_DIR pointing at
the hidden verifier assets, which the agent never sees.
"""

import shutil
from pathlib import Path

from harness import sandbox
from harness.taskspec import TaskSpec


def prepare_workdir(spec: TaskSpec, dest: Path, fixtures_root: Path, verify_root: Path) -> Path:
    dest = Path(dest)
    if spec.fixture:
        shutil.copytree(Path(fixtures_root) / spec.fixture, dest, dirs_exist_ok=True)
    else:
        dest.mkdir(parents=True, exist_ok=True)
    env = {"TASK_VERIFY_DIR": str((Path(verify_root) / spec.id).resolve())}
    for cmd in spec.setup:
        r = sandbox.run(["bash", "-c", cmd], cwd=dest, timeout_s=120, env=env)
        if r.returncode != 0:
            raise RuntimeError(f"{spec.id}: setup command failed ({cmd!r}): {r.stderr.strip()}")
    return dest
