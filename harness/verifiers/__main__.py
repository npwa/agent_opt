"""CLI: python -m harness.verifiers <task id> <workdir> [--answer-file F] [--state-dir D]"""

import argparse
import sys
from pathlib import Path

from harness.taskspec import load_task
from harness.verifiers import VerifyContext, run_verifiers


def main() -> int:
    ap = argparse.ArgumentParser(description="Run a task's verifiers against a finished workdir")
    ap.add_argument("task_id")
    ap.add_argument("workdir", type=Path)
    ap.add_argument("--answer-file", type=Path, help="file holding the agent's final answer")
    ap.add_argument("--state-dir", type=Path, help="where comm-tools stores live")
    ap.add_argument("--tasks", type=Path, default=Path("tasks"))
    ap.add_argument("--verify-root", type=Path, default=Path("fixtures/verify"))
    a = ap.parse_args()
    spec = load_task(a.tasks / f"{a.task_id}.yaml")
    answer = a.answer_file.read_text() if a.answer_file else ""
    ctx = VerifyContext(a.workdir, a.verify_root, spec, answer, a.state_dir)
    verdict = run_verifiers(spec, ctx)
    for r in verdict.results:
        print(f"{'PASS' if r.passed else 'FAIL'} {r.type}: {r.detail}")
    print("VERDICT:", "PASS" if verdict.passed else "FAIL")
    return 0 if verdict.passed else 1


if __name__ == "__main__":
    sys.exit(main())
