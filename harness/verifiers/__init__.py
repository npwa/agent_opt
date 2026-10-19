"""
Executable verifiers (F3).

``run_verifiers(spec, ctx)`` evaluates every verifier of a task against the agent's final
working directory and final answer. Verifiers never modify the original workdir: anything
that executes code works on a scratch copy (see harness.sandbox).
"""

import importlib
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

from harness.taskspec import TaskSpec


@dataclass
class VerifyContext:
    workdir: Path                 # final state of the agent's working directory
    verify_root: Path             # fixtures/verify (hidden assets live in <verify_root>/<task id>/)
    spec: TaskSpec
    final_answer: str = ""        # last assistant message text
    state_dir: Path | None = None  # side-effect stores (comm-tools), defaults to workdir


@dataclass
class VerifierResult:
    type: str
    passed: bool
    detail: str


@dataclass
class TaskVerdict:
    task_id: str
    passed: bool
    results: list[VerifierResult] = field(default_factory=list)

    def summary(self) -> str:
        return "; ".join(f"{r.type}:{'ok' if r.passed else 'FAIL'}({r.detail})" for r in self.results)


Verifier = Callable[[dict, VerifyContext], VerifierResult]


def _registry() -> dict[str, Verifier]:
    from harness.verifiers import code, command, rubric

    return {
        "rubric": rubric.verify,
        "compile_run": code.verify_compile_run,
        "c_test": code.verify_c_test,
        "py_test": code.verify_py_test,
        "command": command.verify,
        "python": verify_python,
    }


def verify_python(params: dict, ctx: VerifyContext) -> VerifierResult:
    module, _, func = params["callable"].partition(":")
    try:
        fn = getattr(importlib.import_module(module), func)
    except (ImportError, AttributeError) as e:
        return VerifierResult("python", False, f"cannot load {params['callable']}: {e}")
    return fn(ctx, **params.get("args", {}))


def run_verifiers(spec: TaskSpec, ctx: VerifyContext) -> TaskVerdict:
    registry = _registry()
    results = []
    for v in spec.verify:
        params = {k: x for k, x in v.items() if k != "type"}
        try:
            results.append(registry[v["type"]](params, ctx))
        except Exception as e:  # a crashing verifier is a failed task, never a crashed run
            results.append(VerifierResult(v["type"], False, f"verifier error: {type(e).__name__}: {e}"))
    return TaskVerdict(spec.id, all(r.passed for r in results), results)
