"""
harness/taskspec.py

Task specification format (F1) and validator.

A task is one YAML file in ``tasks/`` named ``<id>.yaml``. Validation is
two-stage so that CI can run it without a GPU or any model:

* ``validate_spec(data)``   - pure schema check, returns a list of readable errors
* ``check_assets(spec, ..)``- file-system check (fixture dir, verifier files)

Schema (all unknown keys are rejected):

    id: coding_02_fizzbuzz_gen          # == file stem, [a-z0-9_]+
    title: Generate FizzBuzz in C
    category: code_gen                  # one of CATEGORIES
    prompts: [ "..." ]                  # >= 1 user turns, sent in order
    fixture: coding_02_fizzbuzz_gen     # optional dir under fixtures/tasks/, copied to a temp workdir
    setup: ["python3 $TASK_VERIFY_DIR/gen_log.py ."]  # optional shell commands run in the workdir
                                        # before the agent starts; $TASK_VERIFY_DIR = hidden assets
    agent: default                      # OpenCode agent name
    tools: [bash, read, write, edit]    # optional allow-list of built-in tools (omit = all)
    mcp: [searxng]                      # optional MCP servers the task may use
    env: {COMM_TOOLS_MODE: mock}        # optional extra environment for the run
    timeout_s: 900
    expected_tool_sequence: [read, write, bash]   # optional; matched as a subsequence
    tags: [c, short]
    suite: [core]                       # optional named subsets; runner --suite core
    trials: 3                           # optional per-task default n (overrides --n)
    verify:                             # >= 1 verifier, ALL must pass
      - type: compile_run
        ...
"""

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

CATEGORIES = frozenset({
    "code_review", "code_gen", "bugfix", "refactor", "log_triage", "repo_qa",
    "long_context", "reasoning", "comms", "research",
})
TOOLS = frozenset({
    "bash", "read", "write", "edit", "glob", "grep", "list", "webfetch", "task", "todowrite",
})
ID_RE = re.compile(r"^[a-z0-9_]+$")
CALLABLE_RE = re.compile(r"^[A-Za-z_][\w.]*:[A-Za-z_]\w*$")
TIMEOUT_RANGE = (10, 7200)

# verifier type -> (required keys, optional keys)
VERIFIERS: dict[str, tuple[set[str], set[str]]] = {
    "rubric": (set(), {"target", "min_words", "max_words", "required", "forbidden"}),
    "compile_run": ({"sources"}, {"cflags", "expect_lines", "expect_exit", "run_timeout_s"}),
    "c_test": ({"sources", "test_source"}, {"cflags", "run_timeout_s"}),
    "py_test": ({"test_source"}, {"run_timeout_s"}),
    "command": ({"cmd"}, {"expect_exit", "stdout_contains", "run_timeout_s"}),
    "python": ({"callable"}, {"args"}),
}
TOP_REQUIRED = {"id", "title", "category", "prompts", "timeout_s", "verify"}
TOP_OPTIONAL = {"fixture", "setup", "agent", "tools", "mcp", "env", "expected_tool_sequence", "tags", "trials", "suite"}
SUITE_RE = re.compile(r"^[a-z][a-z0-9_]*$")
MAX_TRIALS = 1000


class TaskSpecError(ValueError):
    """Raised with all validation errors of one task file."""

    def __init__(self, source: str, errors: list[str]):
        self.source, self.errors = source, errors
        super().__init__(f"{source}: " + "; ".join(errors))


@dataclass(frozen=True)
class TaskSpec:
    id: str
    title: str
    category: str
    prompts: list[str]
    timeout_s: int
    verify: list[dict[str, Any]]
    fixture: str | None = None
    setup: list[str] = field(default_factory=list)
    agent: str = "default"
    tools: list[str] | None = None
    mcp: list[str] = field(default_factory=list)
    env: dict[str, str] = field(default_factory=dict)
    expected_tool_sequence: list[str] = field(default_factory=list)
    tags: list[str] = field(default_factory=list)
    trials: int | None = None  # per-task default number of trials (overrides the run's --n)
    suite: list[str] = field(default_factory=list)  # named subsets, e.g. core, guard (runner --suite)


def _is_str_list(v: Any, *, non_empty: bool = False) -> bool:
    return (isinstance(v, list) and all(isinstance(x, str) and x.strip() for x in v)
            and (bool(v) or not non_empty))


def _int(v: Any) -> bool:
    return isinstance(v, int) and not isinstance(v, bool)


def _regex_ok(pattern: Any) -> bool:
    if not isinstance(pattern, str) or not pattern:
        return False
    try:
        re.compile(pattern)
    except re.error:
        return False
    return True


def _validate_rubric(i: str, v: dict, errs: list[str]) -> None:
    if not ({"min_words", "max_words", "required", "forbidden"} & v.keys()):
        errs.append(f"{i}: rubric needs at least one of min_words/max_words/required/forbidden")
    for k in ("min_words", "max_words"):
        if k in v and not (_int(v[k]) and v[k] > 0):
            errs.append(f"{i}.{k}: must be a positive integer")
    if {"min_words", "max_words"} <= v.keys() and _int(v["min_words"]) and _int(v["max_words"]) \
            and v["min_words"] > v["max_words"]:
        errs.append(f"{i}: min_words > max_words")
    if "target" in v and not (isinstance(v["target"], str) and v["target"].strip()):
        errs.append(f"{i}.target: must be 'final_answer' or a relative file path")
    for item in v.get("required", []) if isinstance(v.get("required", []), list) else []:
        ok = isinstance(item, dict) and set(item) == {"name", "any_of"} \
            and isinstance(item["name"], str) and isinstance(item["any_of"], list) \
            and item["any_of"] and all(_regex_ok(p) for p in item["any_of"])
        if not ok:
            errs.append(f"{i}.required: each item needs name + non-empty any_of list of valid regexes")
    if "required" in v and not isinstance(v["required"], list):
        errs.append(f"{i}.required: must be a list")
    if "forbidden" in v:
        f = v["forbidden"]
        if not (isinstance(f, list) and all(
                isinstance(x, dict) and set(x) == {"name", "regex"} and _regex_ok(x["regex"])
                for x in f)):
            errs.append(f"{i}.forbidden: each item needs name + valid regex")


def _validate_verifier(idx: int, v: Any, errs: list[str]) -> None:
    i = f"verify[{idx}]"
    if not isinstance(v, dict) or "type" not in v:
        errs.append(f"{i}: must be a mapping with a 'type'")
        return
    vtype = v["type"]
    if vtype not in VERIFIERS:
        errs.append(f"{i}.type: '{vtype}' unknown (expected one of {sorted(VERIFIERS)})")
        return
    required, optional = VERIFIERS[vtype]
    body = {k: x for k, x in v.items() if k != "type"}
    for k in sorted(required - body.keys()):
        errs.append(f"{i}: type '{vtype}' requires '{k}'")
    for k in sorted(body.keys() - required - optional):
        errs.append(f"{i}: unknown key '{k}' for type '{vtype}'")
    if "sources" in body and not _is_str_list(body["sources"], non_empty=True):
        errs.append(f"{i}.sources: must be a non-empty list of file names")
    for k in ("cflags", "expect_lines", "stdout_contains"):
        if k in body and not _is_str_list(body[k]):
            errs.append(f"{i}.{k}: must be a list of strings")
    for k in ("cmd", "test_source"):
        if k in body and not (isinstance(body[k], str) and body[k].strip()):
            errs.append(f"{i}.{k}: must be a non-empty string")
    if "callable" in body and not (isinstance(body["callable"], str)
                                   and CALLABLE_RE.match(body["callable"])):
        errs.append(f"{i}.callable: must look like 'package.module:function'")
    if "expect_exit" in body and not (_int(body["expect_exit"]) and 0 <= body["expect_exit"] <= 255):
        errs.append(f"{i}.expect_exit: must be an integer 0-255")
    if "run_timeout_s" in body and not (_int(body["run_timeout_s"]) and body["run_timeout_s"] > 0):
        errs.append(f"{i}.run_timeout_s: must be a positive integer")
    if "args" in body and not isinstance(body["args"], dict):
        errs.append(f"{i}.args: must be a mapping")
    if vtype == "rubric":
        _validate_rubric(i, body, errs)


def validate_spec(data: Any, *, filename_stem: str | None = None) -> list[str]:
    """Return all schema errors for a parsed task document (empty list = valid)."""
    if not isinstance(data, dict):
        return ["top level must be a mapping"]
    errs: list[str] = []
    for k in sorted(TOP_REQUIRED - data.keys()):
        errs.append(f"missing required key '{k}'")
    for k in sorted(data.keys() - TOP_REQUIRED - TOP_OPTIONAL):
        errs.append(f"unknown key '{k}'")

    tid = data.get("id")
    if "id" in data:
        if not (isinstance(tid, str) and ID_RE.match(tid)):
            errs.append("id: must match [a-z0-9_]+")
        elif filename_stem is not None and tid != filename_stem:
            errs.append(f"id: '{tid}' does not match file name '{filename_stem}.yaml'")
    if "title" in data and not (isinstance(data["title"], str) and data["title"].strip()):
        errs.append("title: must be a non-empty string")
    if "category" in data and data["category"] not in CATEGORIES:
        errs.append(f"category: '{data['category']}' unknown (expected one of {sorted(CATEGORIES)})")
    if "prompts" in data and not _is_str_list(data["prompts"], non_empty=True):
        errs.append("prompts: must be a non-empty list of non-empty strings")
    if "timeout_s" in data:
        t = data["timeout_s"]
        if not (_int(t) and TIMEOUT_RANGE[0] <= t <= TIMEOUT_RANGE[1]):
            errs.append(f"timeout_s: must be an integer in {TIMEOUT_RANGE[0]}..{TIMEOUT_RANGE[1]}")
    if "fixture" in data and not (isinstance(data["fixture"], str)
                                  and ID_RE.match(data["fixture"])):
        errs.append("fixture: must be a directory name under fixtures/tasks/ ([a-z0-9_]+)")
    if "agent" in data and not (isinstance(data["agent"], str) and data["agent"].strip()):
        errs.append("agent: must be a non-empty string")
    if "tools" in data:
        if not _is_str_list(data["tools"]):
            errs.append("tools: must be a list of tool names")
        else:
            for t in data["tools"]:
                if t not in TOOLS:
                    errs.append(f"tools: unknown tool '{t}' (expected one of {sorted(TOOLS)})")
    for k in ("mcp", "tags", "setup"):
        if k in data and not _is_str_list(data[k]):
            errs.append(f"{k}: must be a list of strings")
    if "trials" in data and not (_int(data["trials"]) and 1 <= data["trials"] <= MAX_TRIALS):
        errs.append(f"trials: must be an integer between 1 and {MAX_TRIALS}")
    if "suite" in data:
        if not _is_str_list(data["suite"]):
            errs.append("suite: must be a list of suite names")
        else:
            for s in data["suite"]:
                if not SUITE_RE.match(s):
                    errs.append(f"suite: '{s}' must match [a-z][a-z0-9_]*")
    if "expected_tool_sequence" in data and not _is_str_list(data["expected_tool_sequence"]):
        errs.append("expected_tool_sequence: must be a list of tool names")
    if "env" in data and not (isinstance(data["env"], dict) and all(
            isinstance(k, str) and isinstance(v, str) for k, v in data["env"].items())):
        errs.append("env: must map string names to string values")

    if "verify" in data:
        v = data["verify"]
        if not (isinstance(v, list) and v):
            errs.append("verify: must be a non-empty list of verifiers")
        else:
            for idx, item in enumerate(v):
                _validate_verifier(idx, item, errs)
    return errs


def parse_spec(data: dict) -> TaskSpec:
    return TaskSpec(**{k: data[k] for k in data})


def load_task(path: Path) -> TaskSpec:
    """Load and schema-validate one task file; raises TaskSpecError listing every problem."""
    path = Path(path)
    try:
        data = yaml.safe_load(path.read_text())
    except yaml.YAMLError as e:
        raise TaskSpecError(str(path), [f"invalid YAML: {e}"]) from e
    errs = validate_spec(data, filename_stem=path.stem)
    if errs:
        raise TaskSpecError(str(path), errs)
    return parse_spec(data)


def load_all(tasks_dir: Path) -> list[TaskSpec]:
    return [load_task(p) for p in sorted(Path(tasks_dir).glob("*.yaml"))]


def check_assets(spec: TaskSpec, fixtures_root: Path, verify_root: Path) -> list[str]:
    """File-system checks: fixture dir exists; verifier-side files exist under verify_root/<id>/.

    ``sources`` are produced by the agent inside the workdir, so they are not checked here;
    ``test_source`` is a hidden verifier asset and must exist.
    """
    errs: list[str] = []
    if spec.fixture and not (Path(fixtures_root) / spec.fixture).is_dir():
        errs.append(f"{spec.id}: fixture dir '{fixtures_root}/{spec.fixture}' not found")
    for v in spec.verify:
        if v["type"] in ("c_test", "py_test") and not (Path(verify_root) / spec.id / v["test_source"]).is_file():
            errs.append(f"{spec.id}: test_source '{verify_root}/{spec.id}/{v['test_source']}' not found")
    return errs


def main() -> int:
    import argparse

    ap = argparse.ArgumentParser(description="Validate task specs and their assets")
    ap.add_argument("--tasks", type=Path, default=Path("tasks"))
    ap.add_argument("--fixtures", type=Path, default=Path("fixtures/tasks"))
    ap.add_argument("--verify", type=Path, default=Path("fixtures/verify"))
    args = ap.parse_args()
    bad = 0
    for p in sorted(args.tasks.glob("*.yaml")):
        try:
            spec = load_task(p)
        except TaskSpecError as e:
            bad += 1
            print(f"FAIL {p.name}")
            for err in e.errors:
                print(f"  - {err}")
            continue
        errs = check_assets(spec, args.fixtures, args.verify)
        bad += bool(errs)
        print(f"{'FAIL' if errs else 'ok  '} {p.name}")
        for err in errs:
            print(f"  - {err}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
