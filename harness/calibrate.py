#!/usr/bin/env python3
"""
harness/calibrate.py

M1 step 4 smoke runner: run each task once through OpenCode -> proxy -> Ollama in an
isolated config, verify the result, and record what happened so timeouts and verifier
ambiguity can be calibrated. This is the seed of the M2 trial runner (no randomisation,
no resume logic, no retries).

Isolation: OPENCODE_CONFIG (generated per task) + XDG_* dirs under the output directory
keep the user's global OpenCode config, MCP servers and session database out of the runs.
IMPORTANT: ``opencode run`` blocks forever waiting for stdin unless stdin is /dev/null.

    python -m harness.calibrate [task_id ...] [--model ollama/qwen3.6-32k] [--max-s 2400]
"""

import argparse
import json
import math
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

from harness.manifest import OPENCODE_ISOLATION_ENV
from harness.taskspec import TOOLS, TaskSpec, load_task
from harness.verifiers import VerifyContext, run_verifiers
from harness.workdir import prepare_workdir

ROOT = Path(__file__).resolve().parent.parent
PROXY_URL = "http://localhost:11435/v1"
SEARXNG_URL = "http://localhost:8090"


def build_config(spec: TaskSpec, model: str, comm_tools_dir: Path | None,
                 trial_id: str | None = None, attempt: int | None = None,
                 model_options: dict | None = None, base_url: str = PROXY_URL) -> dict:
    model_id = model.split("/", 1)[1]
    cfg: dict = {
        "permission": "allow",
        "model": model,
        "small_model": model,
        "provider": {"ollama": {
            "npm": "@ai-sdk/openai-compatible", "name": "Ollama",
            "options": {"baseURL": base_url},
            "models": {model_id: {"name": model_id, "tool_call": True}},
        }},
        "mcp": {},
    }
    if trial_id:  # logged (and stripped) by the proxy so every LLM call maps to a trial
        headers = {"X-Trial-Id": trial_id, "X-Task-Id": spec.id}
        if attempt is not None:
            headers["X-Attempt"] = str(attempt)
        cfg["provider"]["ollama"]["options"]["headers"] = headers
    if model_options:  # sampling options such as seed/temperature
        cfg["provider"]["ollama"]["models"][model_id]["options"] = dict(model_options)
    if spec.tools is not None:
        cfg["tools"] = {t: t in spec.tools for t in sorted(TOOLS)}
    if "searxng" in spec.mcp:
        cfg["mcp"]["searxng"] = {"type": "local", "command": ["npx", "-y", "mcp-searxng"],
                                 "environment": {"SEARXNG_URL": SEARXNG_URL}, "enabled": True}
    if "comm-tools" in spec.mcp and comm_tools_dir:
        env = {"COMM_TOOLS_MODE": "mock", **spec.env}
        cfg["mcp"]["comm-tools"] = {"type": "local", "enabled": True, "environment": env,
                                    "command": ["python3", str(comm_tools_dir / "comm_tools_mcp.py")]}
    return cfg


def parse_events(path: Path) -> dict:
    """Summarise an `opencode run --format json` event stream."""
    tools: list[str] = []
    tool_errors = 0
    steps = tokens_in = tokens_out = 0
    session = None
    texts: list[str] = []
    if path.exists():
        for line in path.read_text(errors="replace").splitlines():
            try:
                e = json.loads(line)
            except json.JSONDecodeError:
                continue
            session = session or e.get("sessionID")
            part = e.get("part", {})
            if e["type"] == "tool_use":
                tools.append(part.get("tool", "?"))
                tool_errors += part.get("state", {}).get("status") == "error"
                texts = []  # only text after the last tool call is the final answer
            elif e["type"] == "text":
                texts.append(part.get("text", ""))
            elif e["type"] == "step_finish":
                steps += 1
                tok = part.get("tokens", {})
                tokens_in += tok.get("input", 0)
                tokens_out += tok.get("output", 0)
    return {"session": session, "tools": tools, "tool_errors": tool_errors, "steps": steps,
            "tokens_in": tokens_in, "tokens_out": tokens_out, "final_text": "\n".join(texts).strip()}


def is_subsequence(want: list[str], got: list[str]) -> bool:
    it = iter(got)
    return all(any(w == g or g.endswith("_" + w) for g in it) for w in want)


POLL_S = 2.0


def _wait(p: subprocess.Popen, remaining: float, events: Path, stall_s: float | None, busy) -> int | str:
    """Wait for ``p``: its exit code, or "timeout", or "stalled" (no events, GPU idle and no child working
    for ``stall_s`` seconds; ``busy(pid)`` reports GPU or child activity)."""
    deadline = time.monotonic() + remaining
    size = events.stat().st_size if events.exists() else 0
    last_active = time.monotonic()
    while True:
        left = deadline - time.monotonic()
        if left <= 0:
            return "timeout"
        try:
            return p.wait(timeout=min(POLL_S, left))
        except subprocess.TimeoutExpired:
            pass
        if not stall_s:
            continue
        now = time.monotonic()
        new_size = events.stat().st_size if events.exists() else 0
        if new_size != size or (busy is not None and busy(p.pid)):
            size, last_active = new_size, now
        elif now - last_active >= stall_s:
            return "stalled"


def confined(cmd: list[str], rw: list[Path] | None) -> list[str]:
    """Wrap ``cmd`` so that it runs under harness/landlock.py with only ``rw`` (plus system read-only paths) accessible."""
    if rw is None:
        return cmd
    args = [a for p in rw for a in ("--rw", str(p))]
    return [sys.executable, str(Path(__file__).with_name("landlock.py")), *args, "--", *cmd]  # a script path: cwd is the trial dir


def run_opencode(spec: TaskSpec, work: Path, run_dir: Path, env: dict, model: str,
                 max_s: int, events: Path, stall_s: float | None = None, busy=None,
                 confine: list[Path] | None = None) -> dict:
    session = None
    started = time.monotonic()
    timed_out = stalled = False
    codes = []
    for prompt in spec.prompts:
        cmd = ["opencode", "run", "--title", spec.id, "--format", "json", "--dir", str(work),
               "-m", model]
        if spec.agent != "default":
            cmd += ["--agent", spec.agent]
        if session:
            cmd += ["-s", session]
        cmd.append(prompt)
        remaining = max_s - (time.monotonic() - started)
        if remaining <= 0:
            timed_out = True
            break
        with open(events, "ab") as out, open(run_dir / "stderr.txt", "ab") as err:
            p = subprocess.Popen(confined(cmd, confine), env=env, cwd=work, stdin=subprocess.DEVNULL, stdout=out,
                                 stderr=err, start_new_session=True)
            result = _wait(p, remaining, events, stall_s, busy)
            if isinstance(result, str):
                os.killpg(p.pid, signal.SIGKILL)
                p.wait()
                timed_out, stalled = result == "timeout", result == "stalled"
                codes.append(-9)
                break
            codes.append(result)
        session = session or parse_events(events)["session"]
    return {"elapsed_s": round(time.monotonic() - started, 1), "timed_out": timed_out,
            "stalled": stalled, "exit_codes": codes}


def changed_files(fixture: Path | None, work: Path, limit: int = 20_000) -> dict[str, str]:
    out = {}
    for p in sorted(work.rglob("*")):
        if not p.is_file() or "__pycache__" in p.parts or p.stat().st_size > limit:
            continue
        if b"\0" in p.read_bytes()[:1024]:  # compiled binaries are not worth recording
            continue
        rel = p.relative_to(work)
        orig = fixture / rel if fixture else None
        if orig is None or not orig.is_file() or orig.read_bytes() != p.read_bytes():
            out[str(rel)] = p.read_text(errors="replace")
    return out


def calibrate_one(spec: TaskSpec, args, out: Path, scratch: Path) -> dict:
    run_dir = scratch / spec.id
    shutil.rmtree(run_dir, ignore_errors=True)
    work = prepare_workdir(spec, run_dir / "work", ROOT / "fixtures/tasks", ROOT / "fixtures/verify")
    comm_dir = None
    if "comm-tools" in spec.mcp:
        comm_dir = run_dir / "comm-tools"
        shutil.copytree(ROOT / "agent-config/comm-tools", comm_dir)
    cfg_dir = run_dir / "opencode-config"
    shutil.copytree(ROOT / "agent-config/agent", cfg_dir / "agent")
    cfg_path = run_dir / "opencode.json"
    trial_id = f"cal-{spec.id}-{time.strftime('%Y%m%dT%H%M%S')}"
    cfg_path.write_text(json.dumps(build_config(spec, args.model, comm_dir, trial_id), indent=2))
    xdg = scratch / "xdg"
    env = {**os.environ, "OPENCODE_CONFIG": str(cfg_path), "OPENCODE_CONFIG_DIR": str(cfg_dir),
           "XDG_CONFIG_HOME": str(xdg / "config"), "XDG_DATA_HOME": str(xdg / "data"),
           "XDG_STATE_HOME": str(xdg / "state"), "XDG_CACHE_HOME": str(xdg / "cache"),
           **OPENCODE_ISOLATION_ENV}
    (out / "proxy" / "current_task.txt").write_text(spec.id)

    rec_dir = out / spec.id
    shutil.rmtree(rec_dir, ignore_errors=True)
    rec_dir.mkdir(parents=True)
    events = rec_dir / "events.jsonl"
    run = run_opencode(spec, work, run_dir, env, args.model, args.max_s, events)
    ev = parse_events(events)
    ctx = VerifyContext(workdir=work, verify_root=ROOT / "fixtures/verify", spec=spec,
                        final_answer=ev["final_text"], state_dir=comm_dir)
    verdict = run_verifiers(spec, ctx)
    fixture = ROOT / "fixtures/tasks" / spec.fixture if spec.fixture else None
    record = {
        "task": spec.id, "trial_id": trial_id, "model": args.model, **run,
        "steps": ev["steps"], "tokens_in": ev["tokens_in"], "tokens_out": ev["tokens_out"],
        "tool_sequence": ev["tools"], "tool_errors": ev["tool_errors"],
        "expected_tool_sequence": spec.expected_tool_sequence,
        "expected_sequence_matched": is_subsequence(spec.expected_tool_sequence, ev["tools"]),
        "spec_timeout_s": spec.timeout_s,
        "suggested_timeout_s": max(120, int(math.ceil(run["elapsed_s"] * 3 / 60) * 60)),
        "verdict_passed": verdict.passed,
        "verifier_results": [r.__dict__ for r in verdict.results],
        "final_answer": ev["final_text"],
        "files": sorted(changed_files(fixture, work)),
    }
    for name, text in changed_files(fixture, work).items():
        dest = rec_dir / "artifacts" / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(text)
    (rec_dir / "record.json").write_text(json.dumps(record, indent=2))
    return record


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("tasks", nargs="*", help="task ids (default: all)")
    ap.add_argument("--model", default="ollama/qwen3.6-32k")
    ap.add_argument("--max-s", type=int, default=2400)
    ap.add_argument("--out", type=Path, default=ROOT / "results/v1/calibration")
    ap.add_argument("--scratch", type=Path, required=True, help="directory for workdirs (not committed)")
    args = ap.parse_args()
    (args.out / "proxy").mkdir(parents=True, exist_ok=True)
    specs = [load_task(p) for p in sorted((ROOT / "tasks").glob("*.yaml"))]
    if args.tasks:
        specs = [s for s in specs if s.id in args.tasks]
    for spec in specs:
        print(f"[{time.strftime('%H:%M:%S')}] START {spec.id}", flush=True)
        r = calibrate_one(spec, args, args.out, args.scratch)
        print(f"[{time.strftime('%H:%M:%S')}] DONE  {spec.id}: {r['elapsed_s']}s steps={r['steps']} "
              f"timed_out={r['timed_out']} passed={r['verdict_passed']} "
              f"seq_ok={r['expected_sequence_matched']}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
