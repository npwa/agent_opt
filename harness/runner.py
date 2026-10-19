#!/usr/bin/env python3
"""
harness/runner.py

Trial runner (F4): runs every (configuration x task x repeat) through OpenCode -> proxy ->
Ollama, verifies it, and writes one self-contained record per trial.

Design rules
* **Randomised order**: the full trial list is shuffled with a seeded RNG so thermal, cache and
  time-of-day drift is spread over all cells instead of biasing the last one.
* **Resumable**: ``<out>/plan.json`` freezes the trial order; a trial is done once
  ``<out>/trials/<trial_id>/record.json`` exists. Interrupt at any time and run the same
  command again.
* **Retries only for harness failures** (OpenCode crash, no events, upstream/Ollama error,
  verifier crash, setup failure). A wrong answer or a timeout *with progress* is a measured
  outcome and is **never** retried.
* Per-trial seed (when the config asks for it) is derived from the run seed and the trial id.

    python -m harness.runner run --configs configs/baseline_32k.yaml --n 10 --seed 1 \
        --out results/v1/baseline --scratch /path/to/scratch
    python -m harness.runner status --out results/v1/baseline
"""

import argparse
import hashlib
import json
import os
import random
import shutil
import sys
import time
import urllib.request
from dataclasses import dataclass, field
from datetime import UTC, datetime
from pathlib import Path

import yaml

from harness import calibrate, manifest
from harness.taskspec import TaskSpec, load_task
from harness.telemetry import GpuSampler
from harness.verifiers import VerifyContext, run_verifiers
from harness.workdir import prepare_workdir

ROOT = Path(__file__).resolve().parent.parent
FINAL_STATUSES = {"pass", "fail", "timeout", "harness_failed"}
CONFIG_KEYS = {"id", "model", "description", "seed_mode", "options"}


class PlanMismatchError(RuntimeError):
    """The existing plan.json in the output directory does not match the requested run."""


@dataclass(frozen=True)
class RunConfig:
    id: str
    model: str
    description: str = ""
    seed_mode: str = "per_trial"
    options: dict = field(default_factory=dict)


def load_config(path: Path) -> RunConfig:
    data = yaml.safe_load(Path(path).read_text())
    if not isinstance(data, dict):
        raise ValueError(f"{path}: must be a mapping")
    extra = data.keys() - CONFIG_KEYS
    missing = {"id", "model"} - data.keys()
    if extra or missing:
        raise ValueError(f"{path}: unknown keys {sorted(extra)}, missing keys {sorted(missing)}")
    if "/" not in str(data["model"]):
        raise ValueError(f"{path}: model must look like provider/name")
    if data.get("seed_mode", "per_trial") not in ("per_trial", "none"):
        raise ValueError(f"{path}: seed_mode must be per_trial or none")
    if not str(data["id"]).replace("_", "").replace("-", "").isalnum():
        raise ValueError(f"{path}: id must be alphanumeric/_/-")
    return RunConfig(**data)


# ---- planning ------------------------------------------------------------------------------

def trial_id(config_id: str, task_id: str, k: int) -> str:
    return f"{config_id}__{task_id}__{k:02d}"


def trial_seed(run_seed: int, tid: str) -> int:
    return int(hashlib.sha256(f"{run_seed}:{tid}".encode()).hexdigest()[:8], 16) % (2**31)


def spec_hash(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()[:12]


def build_plan(configs: list[RunConfig], specs: list[TaskSpec], n: int, run_seed: int,
               tasks_dir: Path) -> dict:
    trials = [{"trial_id": trial_id(c.id, s.id, k), "config": c.id, "task": s.id, "k": k}
              for c in configs for s in specs for k in range(n)]
    random.Random(run_seed).shuffle(trials)
    return {
        "created_utc": datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "run_seed": run_seed, "n": n,
        "configs": {c.id: c.__dict__ for c in configs},
        "tasks": {s.id: spec_hash(tasks_dir / f"{s.id}.yaml") for s in specs},
        "order": trials,
    }


def load_or_create_plan(out: Path, plan: dict, force: bool = False) -> dict:
    path = out / "plan.json"
    if path.exists():
        old = json.loads(path.read_text())
        same = all(old[k] == plan[k] for k in ("run_seed", "n", "configs", "tasks"))
        if not same and not force:
            raise PlanMismatchError(
                f"{path} was created with different seed/n/configs/tasks; use a new --out "
                "directory (or --force-plan to overwrite the plan and keep finished trials)")
        if same:
            return old
    out.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(plan, indent=2))
    return plan


def is_done(out: Path, tid: str) -> bool:
    rec = out / "trials" / tid / "record.json"
    if not rec.exists():
        return False
    try:
        return json.loads(rec.read_text()).get("status") in FINAL_STATUSES
    except json.JSONDecodeError:
        return False


# ---- classification ---------------------------------------------------------------------------

def proxy_errors(metrics: Path, tid: str, attempt: int) -> list[int]:
    """HTTP statuses != 200 the proxy saw for this trial attempt (Ollama/upstream trouble)."""
    if not metrics.exists():
        return []
    bad = []
    for line in metrics.read_text().splitlines():
        try:
            r = json.loads(line)
        except json.JSONDecodeError:
            continue
        if r.get("trial_id") == tid and str(r.get("attempt")) == str(attempt):
            # aborted requests are expected when a timed-out trial is killed - not an error
            if r.get("status", 200) != 200:
                bad.append(r.get("status", 0))
    return bad


def classify(run: dict, ev: dict, verdict, upstream_bad: list[int]) -> tuple[str, str | None]:
    """-> (status, harness_reason). status in pass/fail/timeout/harness_error."""
    if upstream_bad:
        return "harness_error", f"upstream_error:{sorted(set(upstream_bad))}"
    if ev["steps"] == 0 and not ev["tools"] and not ev["final_text"]:
        # the agent never produced anything: hang or crash, not a wrong answer
        reason = "timeout_without_events" if run["timed_out"] else "no_events"
        return "harness_error", reason
    if any(r.detail.startswith("verifier error") for r in verdict.results):
        return "harness_error", "verifier_crash"
    if run["timed_out"]:
        return "timeout", None
    return ("pass" if verdict.passed else "fail"), None


# ---- executing one attempt ------------------------------------------------------------------

def preflight(specs: list[TaskSpec], proxy_url: str = "http://localhost:11435",
              searxng_url: str = "http://localhost:8090") -> list[str]:
    problems = []

    def reachable(url: str) -> bool:
        try:
            with urllib.request.urlopen(url, timeout=5) as r:
                return r.status == 200
        except (OSError, ValueError):
            return False

    if not reachable(f"{proxy_url}/v1/models"):
        problems.append(f"proxy/Ollama not reachable at {proxy_url} "
                        "(start: python -m uvicorn harness.proxy:app --port 11435)")
    if any("searxng" in s.mcp for s in specs) and not reachable(searxng_url):
        problems.append(f"SearXNG not reachable at {searxng_url} (docker start searxng)")
    return problems


def run_attempt(spec: TaskSpec, cfg: RunConfig, tid: str, attempt: int, seed: int | None,
                trial_dir: Path, scratch: Path, metrics: Path) -> dict:
    run_dir = scratch / tid
    shutil.rmtree(run_dir, ignore_errors=True)
    events = trial_dir / "events.jsonl"
    events.unlink(missing_ok=True)
    started_utc = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
    wall_start = time.time()
    try:
        work = prepare_workdir(spec, run_dir / "work", ROOT / "fixtures/tasks",
                               ROOT / "fixtures/verify")
    except RuntimeError as e:
        return {"status": "harness_error", "reason": f"setup_failed: {e}", "attempt": attempt,
                "started_utc": started_utc}
    comm_dir = None
    if "comm-tools" in spec.mcp:
        comm_dir = run_dir / "comm-tools"
        shutil.copytree(ROOT / "agent-config/comm-tools", comm_dir)
    cfg_dir = run_dir / "opencode-config"
    shutil.copytree(ROOT / "agent-config/agent", cfg_dir / "agent")
    options = dict(cfg.options)
    if cfg.seed_mode == "per_trial" and seed is not None:
        options["seed"] = seed
    opencode_json = run_dir / "opencode.json"
    opencode_json.write_text(json.dumps(
        calibrate.build_config(spec, cfg.model, comm_dir, tid, attempt, options), indent=2))
    xdg = scratch / "xdg"
    env = {**os.environ, "OPENCODE_CONFIG": str(opencode_json), "OPENCODE_CONFIG_DIR": str(cfg_dir),
           "XDG_CONFIG_HOME": str(xdg / "config"), "XDG_DATA_HOME": str(xdg / "data"),
           "XDG_STATE_HOME": str(xdg / "state"), "XDG_CACHE_HOME": str(xdg / "cache")}

    run = calibrate.run_opencode(spec, work, run_dir, env, cfg.model, spec.timeout_s, events)
    wall_end = time.time()
    ev = calibrate.parse_events(events)
    ctx = VerifyContext(workdir=work, verify_root=ROOT / "fixtures/verify", spec=spec,
                        final_answer=ev["final_text"], state_dir=comm_dir)
    verdict = run_verifiers(spec, ctx)
    status, reason = classify(run, ev, verdict, proxy_errors(metrics, tid, attempt))
    fixture = ROOT / "fixtures/tasks" / spec.fixture if spec.fixture else None
    files = calibrate.changed_files(fixture, work)
    for name, text in files.items():
        dest = trial_dir / "artifacts" / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(text)
    shutil.rmtree(run_dir, ignore_errors=True)
    return {
        "status": status, "reason": reason, "attempt": attempt, "started_utc": started_utc,
        "wall_start": wall_start, "wall_end": wall_end, **run,
        "steps": ev["steps"], "tokens_in": ev["tokens_in"], "tokens_out": ev["tokens_out"],
        "tool_sequence": ev["tools"], "tool_errors": ev["tool_errors"],
        "expected_sequence_matched": calibrate.is_subsequence(spec.expected_tool_sequence, ev["tools"]),
        "verdict_passed": verdict.passed,
        "verifier_results": [r.__dict__ for r in verdict.results],
        "final_answer": ev["final_text"], "files": sorted(files),
    }


def run_trial(spec: TaskSpec, cfg: RunConfig, item: dict, run_seed: int, out: Path,
              scratch: Path, metrics: Path, max_retries: int,
              attempt_fn=run_attempt) -> dict:
    tid = item["trial_id"]
    trial_dir = out / "trials" / tid
    trial_dir.mkdir(parents=True, exist_ok=True)
    seed = trial_seed(run_seed, tid) if cfg.seed_mode == "per_trial" else None
    attempts = []
    final: dict = {}
    for attempt in range(1, max_retries + 2):
        res = attempt_fn(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics)
        attempts.append({k: res.get(k) for k in ("attempt", "status", "reason", "started_utc")})
        if res["status"] != "harness_error":
            final = res
            break
        events = trial_dir / "events.jsonl"
        if events.exists():
            events.rename(trial_dir / f"events.attempt{attempt}.jsonl")
    else:
        final = {"status": "harness_failed", "reason": attempts[-1]["reason"]}
    record = {
        "trial_id": tid, "task": spec.id, "config": cfg.id, "model": cfg.model, "k": item["k"],
        "seed": seed, "run_seed": run_seed, **final, "attempts": attempts,
    }
    tmp = trial_dir / "record.json.tmp"
    tmp.write_text(json.dumps(record, indent=2))
    tmp.rename(trial_dir / "record.json")  # atomic: record.json exists only for finished trials
    return record


# ---- driver ------------------------------------------------------------------------------------

def status_table(out: Path) -> str:
    plan = json.loads((out / "plan.json").read_text())
    counts: dict[str, int] = {}
    pending = 0
    for item in plan["order"]:
        rec = out / "trials" / item["trial_id"] / "record.json"
        if rec.exists():
            st = json.loads(rec.read_text())["status"]
            counts[st] = counts.get(st, 0) + 1
        else:
            pending += 1
    done = sum(counts.values())
    parts = ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))
    return f"{done}/{len(plan['order'])} trials done ({parts or 'none'}), {pending} pending"


def run_all(configs: list[RunConfig], specs: list[TaskSpec], n: int, run_seed: int, out: Path,
            scratch: Path, metrics: Path, max_retries: int = 2, limit: int | None = None,
            force_plan: bool = False, tasks_dir: Path = ROOT / "tasks", attempt_fn=run_attempt,
            log=print) -> dict:
    plan = load_or_create_plan(out, build_plan(configs, specs, n, run_seed, tasks_dir), force_plan)
    by_cfg = {c.id: c for c in configs}
    by_task = {s.id: s for s in specs}
    if not (out / "platform.yaml").exists():
        manifest.write_manifest(out / "platform.yaml")
    ran = 0
    for item in plan["order"]:
        if is_done(out, item["trial_id"]):
            continue
        if limit is not None and ran >= limit:
            break
        log(f"[{time.strftime('%H:%M:%S')}] {item['trial_id']}")
        rec = run_trial(by_task[item["task"]], by_cfg[item["config"]], item, run_seed, out,
                        scratch, metrics, max_retries, attempt_fn)
        ran += 1
        log(f"    -> {rec['status']}"
            f"{' (' + str(rec.get('reason')) + ')' if rec.get('reason') else ''}"
            f" {rec.get('elapsed_s', '-')}s attempts={len(rec['attempts'])}")
    return plan


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--configs", type=Path, nargs="+", required=True)
    r.add_argument("--tasks", nargs="*", help="task ids (default: all)")
    r.add_argument("--n", type=int, default=10)
    r.add_argument("--seed", type=int, default=1)
    r.add_argument("--out", type=Path, required=True)
    r.add_argument("--scratch", type=Path, required=True)
    r.add_argument("--proxy-data", type=Path, help="proxy data dir holding metrics.jsonl "
                   "(default: <out>/proxy)")
    r.add_argument("--max-retries", type=int, default=2)
    r.add_argument("--limit", type=int, help="run at most this many pending trials, then stop")
    r.add_argument("--force-plan", action="store_true")
    r.add_argument("--rerun-harness-failed", action="store_true",
                   help="delete records of trials that ended as harness_failed so they run again")
    r.add_argument("--gpu-hz", type=float, default=10.0, help="NVML sampling rate (F8)")
    r.add_argument("--no-gpu", action="store_true", help="do not sample the GPU")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--no-preflight", action="store_true")
    st = sub.add_parser("status")
    st.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()

    if a.cmd == "status":
        print(status_table(a.out))
        return 0
    specs = [load_task(p) for p in sorted((ROOT / "tasks").glob("*.yaml"))]
    if a.tasks:
        specs = [s for s in specs if s.id in a.tasks]
    configs = [load_config(p) for p in a.configs]
    metrics = (a.proxy_data or a.out / "proxy") / "metrics.jsonl"
    if a.dry_run:
        plan = build_plan(configs, specs, a.n, a.seed, ROOT / "tasks")
        for item in plan["order"]:
            print(item["trial_id"])
        print(f"{len(plan['order'])} trials")
        return 0
    if not a.no_preflight and (problems := preflight(specs)):
        print("preflight failed:\n  - " + "\n  - ".join(problems), file=sys.stderr)
        return 2
    if a.rerun_harness_failed:
        for rec in (a.out / "trials").glob("*/record.json") if (a.out / "trials").exists() else []:
            if json.loads(rec.read_text()).get("status") == "harness_failed":
                rec.unlink()
    sampler = None if a.no_gpu else GpuSampler(a.out / "gpu_samples.csv", a.gpu_hz)
    try:
        if sampler:
            sampler.start()
        run_all(configs, specs, a.n, a.seed, a.out, a.scratch, metrics, a.max_retries,
                a.limit, a.force_plan)
    except PlanMismatchError as e:
        print(e, file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        print("\ninterrupted - run the same command again to resume", file=sys.stderr)
        return 130
    finally:
        if sampler:
            sampler.stop()
            if sampler.error:
                print(f"WARNING: GPU sampler stopped early: {sampler.error!r}", file=sys.stderr)
    print(status_table(a.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
