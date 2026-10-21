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
import math
import os
import random
import re
import shutil
import sys
import time
import urllib.request
from dataclasses import dataclass, field
from datetime import UTC, datetime
from pathlib import Path

import yaml

from harness import calibrate, manifest, stall
from harness import server as serverlib
from harness.manifest import OPENCODE_ISOLATION_ENV
from harness.taskspec import TaskSpec, load_task
from harness.telemetry import GpuSampler
from harness.verifiers import VerifyContext, run_verifiers
from harness.workdir import prepare_workdir

ROOT = Path(__file__).resolve().parent.parent
FINAL_STATUSES = {"pass", "fail", "timeout", "harness_failed"}
CONFIG_KEYS = {"id", "model", "description", "seed_mode", "options", "server"}
STALL_S = 300.0  # no events + idle GPU + no working child for this long = stalled trial (0 disables); set by --stall-s


class PlanMismatchError(RuntimeError):
    """The existing plan.json in the output directory does not match the requested run."""


@dataclass(frozen=True)
class RunConfig:
    id: str
    model: str
    description: str = ""
    seed_mode: str = "per_trial"
    options: dict = field(default_factory=dict)
    server: dict | None = None  # settings of a private Ollama server for this config (harness/server.py)


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
    if "server" in data and (errs := serverlib.validate_server(data["server"])):
        raise ValueError(f"{path}: " + "; ".join(errs))
    return RunConfig(**data)


# ---- planning ------------------------------------------------------------------------------

def trial_id(config_id: str, task_id: str, k: int, instrumented: bool = True) -> str:
    return f"{config_id}__{task_id}__{'' if instrumented else 'p'}{k:02d}"


def trial_seed(run_seed: int, tid: str) -> int:
    return int(hashlib.sha256(f"{run_seed}:{tid}".encode()).hexdigest()[:8], 16) % (2**31)


def spec_hash(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()[:12]


def trials_for(spec: TaskSpec, n: int, use_task_trials: bool = True) -> int:
    """Instrumented trials of a task: its own ``trials`` field if it has one (and is honoured), else ``n``."""
    return spec.trials if use_task_trials and spec.trials else n


def build_plan(configs: list[RunConfig], specs: list[TaskSpec], n: int, run_seed: int,
               tasks_dir: Path, parity_fraction: float = 0.0, use_task_trials: bool = True) -> dict:
    """Instrumented trials plus, per (config, task) cell, ceil(parity_fraction * trials) trials that
    bypass the proxy (talk to Ollama directly) for the instrumentation parity check (F7). A task's
    own ``trials`` field overrides ``n`` (e.g. regression-guard tasks run fewer times)."""
    per_task = {s.id: trials_for(s, n, use_task_trials) for s in specs}
    trials = [{"trial_id": trial_id(c.id, s.id, k, inst), "config": c.id, "task": s.id, "k": k,
               "instrumented": inst}
              for c in configs for s in specs
              for inst, count in ((True, per_task[s.id]),
                                  (False, math.ceil(parity_fraction * per_task[s.id]) if parity_fraction > 0 else 0))
              for k in range(count)]
    random.Random(run_seed).shuffle(trials)
    # configurations that need their own Ollama server run in blocks (a server restart per trial would be
    # absurd); the block order is seeded and trials stay shuffled inside a block. With a single group
    # (no server settings) this is a no-op, so the order of plain runs is unchanged.
    group_of = {c.id: serverlib.group_key(c.server) for c in configs}
    groups = sorted(set(group_of.values()))
    random.Random(f"groups:{run_seed}").shuffle(groups)
    if len(groups) > 1:
        trials = [t for g in groups for t in trials if group_of[t["config"]] == g]
    return {
        "created_utc": datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "run_seed": run_seed, "n": n, "parity_fraction": parity_fraction, "trials_per_task": per_task,
        "configs": {c.id: c.__dict__ for c in configs},
        "tasks": {s.id: spec_hash(tasks_dir / f"{s.id}.yaml") for s in specs},
        "order": trials,
    }


def load_or_create_plan(out: Path, plan: dict, force: bool = False) -> dict:
    path = out / "plan.json"
    if path.exists():
        old = json.loads(path.read_text())
        same = (all(old[k] == plan[k] for k in ("run_seed", "n", "configs", "tasks"))
                and old.get("parity_fraction", 0.0) == plan["parity_fraction"]
                and old.get("trials_per_task", {t: old["n"] for t in old["tasks"]}) == plan["trials_per_task"])
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
    # (upstream_bad is always empty for uninstrumented trials: there is no proxy to ask)
    """-> (status, harness_reason). status in pass/fail/timeout/harness_error."""
    if run.get("stalled"):
        return "harness_error", "stalled"  # OpenCode waited on a response that never came: never an outcome
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
              searxng_url: str = "http://localhost:8090", need_direct: bool = False,
              ollama_url: str = "http://localhost:11434", private_server: bool = False) -> list[str]:
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
    if need_direct and not reachable(f"{ollama_url}/api/tags"):
        problems.append(f"Ollama not reachable at {ollama_url} (needed for uninstrumented parity trials)")
    if private_server:
        if shutil.which("ollama") is None:
            problems.append("`ollama` not found on PATH (needed for configurations with a `server:` section)")
        if not reachable(f"{ollama_url}/api/tags"):
            problems.append(f"system Ollama not reachable at {ollama_url} (it owns the model store the private server uses)")
        if reachable(f"http://{serverlib.PRIVATE_HOST}/api/tags"):
            problems.append(f"{serverlib.PRIVATE_HOST} is already in use (private Ollama for `server:` configurations)")
    if any("searxng" in s.mcp for s in specs) and not reachable(searxng_url):
        problems.append(f"SearXNG not reachable at {searxng_url} (docker start searxng)")
    return problems


DEFAULT_DIRECT_URL = "http://localhost:11434/v1"
OLLAMA_DIRECT_URL = DEFAULT_DIRECT_URL  # where uninstrumented (parity) trials go; run_all points it at a managed server


COMM_STATE_FILES = ("email_store.json", "calendar_store.json")


def save_comm_state(comm_dir: Path | None, trial_dir: Path) -> None:
    """Keep the mock comm-tools stores: the comms verifier reads them, so replay needs them."""
    if comm_dir is None:
        return
    for name in COMM_STATE_FILES:
        if (comm_dir / name).is_file():
            dest = trial_dir / "artifacts" / "comm_state"
            dest.mkdir(parents=True, exist_ok=True)
            shutil.copy(comm_dir / name, dest / name)


def run_attempt(spec: TaskSpec, cfg: RunConfig, tid: str, attempt: int, seed: int | None,
                trial_dir: Path, scratch: Path, metrics: Path, instrumented: bool = True) -> dict:
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
        calibrate.build_config(
            spec, cfg.model, comm_dir, tid if instrumented else None, attempt, options,
            base_url=calibrate.PROXY_URL if instrumented else OLLAMA_DIRECT_URL), indent=2))
    xdg = scratch / "xdg"
    env = {**os.environ, "OPENCODE_CONFIG": str(opencode_json), "OPENCODE_CONFIG_DIR": str(cfg_dir),
           "XDG_CONFIG_HOME": str(xdg / "config"), "XDG_DATA_HOME": str(xdg / "data"),
           "XDG_STATE_HOME": str(xdg / "state"), "XDG_CACHE_HOME": str(xdg / "cache"),
           **OPENCODE_ISOLATION_ENV}

    run = calibrate.run_opencode(spec, work, run_dir, env, cfg.model, spec.timeout_s, events,
                                 stall_s=STALL_S, busy=stall.make_probe() if STALL_S else None)
    wall_end = time.time()
    ev = calibrate.parse_events(events)
    ctx = VerifyContext(workdir=work, verify_root=ROOT / "fixtures/verify", spec=spec,
                        final_answer=ev["final_text"], state_dir=comm_dir)
    verdict = run_verifiers(spec, ctx)
    upstream_bad = proxy_errors(metrics, tid, attempt) if instrumented else []
    status, reason = classify(run, ev, verdict, upstream_bad)
    fixture = ROOT / "fixtures/tasks" / spec.fixture if spec.fixture else None
    files = calibrate.changed_files(fixture, work)
    for name, text in files.items():
        dest = trial_dir / "artifacts" / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(text)
    save_comm_state(comm_dir, trial_dir)
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
        res = attempt_fn(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics,
                         instrumented=item.get("instrumented", True))
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
        "instrumented": item.get("instrumented", True),
        "prev_trial_id": item.get("prev_trial_id"), "prev_task": item.get("prev_task"),
        "seed": seed, "run_seed": run_seed, **final, "attempts": attempts,
    }
    tmp = trial_dir / "record.json.tmp"
    tmp.write_text(json.dumps(record, indent=2))
    tmp.rename(trial_dir / "record.json")  # atomic: record.json exists only for finished trials
    return record


# ---- driver ------------------------------------------------------------------------------------

def status_table(out: Path, by_task: bool = False) -> str:
    plan = json.loads((out / "plan.json").read_text())
    counts: dict[str, int] = {}
    pending = 0
    tasks: dict[str, list[int]] = {}  # task -> [planned, done]
    for item in plan["order"]:
        rec = out / "trials" / item["trial_id"] / "record.json"
        row = tasks.setdefault(item["task"], [0, 0])
        row[0] += 1
        if rec.exists():
            st = json.loads(rec.read_text())["status"]
            counts[st] = counts.get(st, 0) + 1
            row[1] += 1
        else:
            pending += 1
    done = sum(counts.values())
    parts = ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))
    line = f"{done}/{len(plan['order'])} trials done ({parts or 'none'}), {pending} pending"
    if by_task:
        line += "\n" + "\n".join(f"  {t}: {d}/{p}" for t, (p, d) in sorted(tasks.items()))
    return line


def _set_direct_url(url: str) -> None:
    global OLLAMA_DIRECT_URL
    OLLAMA_DIRECT_URL = url


def key_slug(config_id: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]", "_", config_id)


def recorded_servers(out: Path) -> set[str]:
    """Configurations whose server facts are already in platform.yaml (resumed runs record them once)."""
    p = out / "platform.yaml"
    try:
        return set((yaml.safe_load(p.read_text()) or {}).get("servers", {}))
    except (OSError, yaml.YAMLError):
        return set()


def run_all(configs: list[RunConfig], specs: list[TaskSpec], n: int, run_seed: int, out: Path,
            scratch: Path, metrics: Path, max_retries: int = 2, limit: int | None = None,
            force_plan: bool = False, tasks_dir: Path = ROOT / "tasks", attempt_fn=run_attempt,
            log=print, parity_fraction: float = 0.0, use_task_trials: bool = True,
            server_factory=None) -> dict:
    """Run the pending trials of the plan. Configurations with a ``server`` section run on a private Ollama
    server that is started when their block of trials begins and stopped when it ends (``server_factory``
    builds it, default harness.server.ManagedServer; the proxy is pointed at it via upstream.txt)."""
    server_factory = server_factory or serverlib.ManagedServer
    plan = load_or_create_plan(
        out, build_plan(configs, specs, n, run_seed, tasks_dir, parity_fraction, use_task_trials), force_plan)
    by_cfg = {c.id: c for c in configs}
    by_task = {s.id: s for s in specs}
    if not (out / "platform.yaml").exists():
        manifest.write_manifest(out / "platform.yaml")
    ran = 0
    order = plan["order"]
    upstream = metrics.parent / "upstream.txt"
    active, active_key, facts_by_key = None, None, {}
    recorded = recorded_servers(out)
    try:
        for pos, item in enumerate(order):
            if is_done(out, item["trial_id"]):
                continue
            if limit is not None and ran >= limit:
                break
            cfg = by_cfg[item["config"]]
            key = serverlib.group_key(cfg.server)
            if key != active_key:  # next block of trials: switch the Ollama server
                if active:
                    active.stop()
                    active = None
                upstream.unlink(missing_ok=True)
                _set_direct_url(DEFAULT_DIRECT_URL)
                if cfg.server is not None:
                    log(f"starting private Ollama for {cfg.id}: {cfg.server}")
                    active = server_factory(cfg.server, cfg.model.split("/", 1)[1], out / "servers" / f"{key_slug(cfg.id)}.log")
                    facts_by_key[key] = active.start()
                    upstream.parent.mkdir(parents=True, exist_ok=True)
                    upstream.write_text(active.url)
                    _set_direct_url(active.v1)
                    if facts_by_key[key].get("not_as_requested"):
                        log("WARNING: server did not apply every setting: " + "; ".join(facts_by_key[key]["not_as_requested"]))
                active_key = key
            if cfg.server is not None and cfg.id not in recorded:
                serverlib.record_server(out / "platform.yaml", cfg.id, facts_by_key[key])
                recorded.add(cfg.id)
            # trial order is randomised but back-to-back, so the previous trial's prompt cache can speed this
            # trial up (first-call prefill): record the predecessor so analyses can use it as a covariate
            item = {**item, "prev_trial_id": order[pos - 1]["trial_id"] if pos else None,
                    "prev_task": order[pos - 1]["task"] if pos else None}
            log(f"[{time.strftime('%H:%M:%S')}] {item['trial_id']}")
            rec = run_trial(by_task[item["task"]], cfg, item, run_seed, out, scratch, metrics, max_retries, attempt_fn)
            ran += 1
            log(f"    -> {rec['status']}"
                f"{' (' + str(rec.get('reason')) + ')' if rec.get('reason') else ''}"
                f" {rec.get('elapsed_s', '-')}s attempts={len(rec['attempts'])}")
    finally:
        if active:
            active.stop()
        upstream.unlink(missing_ok=True)
        _set_direct_url(DEFAULT_DIRECT_URL)
    return plan


def main() -> int:
    global STALL_S
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--configs", type=Path, nargs="+", required=True)
    r.add_argument("--tasks", nargs="*", help="task ids (default: all)")
    r.add_argument("--suite", nargs="*", help="only tasks in these suites (task field `suite`, e.g. core guard); "
                   "combined with --tasks by intersection")
    r.add_argument("--ignore-task-trials", action="store_true",
                   help="use --n for every task, ignoring per-task `trials` (e.g. for a uniform baseline)")
    r.add_argument("--n", type=int, default=10)
    r.add_argument("--seed", type=int, default=1)
    r.add_argument("--out", type=Path, required=True)
    r.add_argument("--scratch", type=Path, required=True)
    r.add_argument("--proxy-data", type=Path, help="proxy data dir holding metrics.jsonl "
                   "(default: <out>/proxy)")
    r.add_argument("--parity-fraction", type=float, default=0.0,
                   help="per (config, task) cell run ceil(fraction * n) extra trials directly against "
                        "Ollama (no proxy) and compare them with the instrumented ones (F7); "
                        "plan suggests 0.2")
    r.add_argument("--max-retries", type=int, default=2)
    r.add_argument("--stall-s", type=float, default=STALL_S,
                   help="end a trial as a harness failure after this many seconds without events, GPU activity "
                        "or a working child process (default %(default)s; 0 disables)")
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
    st.add_argument("--by-task", action="store_true", help="also list done/planned trials per task")
    a = ap.parse_args()

    if a.cmd == "status":
        print(status_table(a.out, a.by_task))
        return 0
    specs = [load_task(p) for p in sorted((ROOT / "tasks").glob("*.yaml"))]
    known = sorted({x for s in specs for x in s.suite})  # all suites, before --tasks narrows the list
    if a.tasks:
        specs = [s for s in specs if s.id in a.tasks]
    if a.suite:
        if unknown := [x for x in a.suite if x not in known]:
            print(f"unknown suite(s) {unknown}; known: {known}", file=sys.stderr)
            return 2
        specs = [s for s in specs if set(s.suite) & set(a.suite)]
    if not specs:
        print("no tasks selected", file=sys.stderr)
        return 2
    configs = [load_config(p) for p in a.configs]
    metrics = (a.proxy_data or a.out / "proxy") / "metrics.jsonl"
    if a.dry_run:
        plan = build_plan(configs, specs, a.n, a.seed, ROOT / "tasks", a.parity_fraction,
                          not a.ignore_task_trials)
        for item in plan["order"]:
            print(item["trial_id"])
        print("trials per task: " + ", ".join(f"{t}={k}" for t, k in plan["trials_per_task"].items()))
        print(f"{len(plan['order'])} trials")
        return 0
    if not a.no_preflight and (problems := preflight(specs, need_direct=a.parity_fraction > 0,
                                                      private_server=any(c.server is not None for c in configs))):
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
        STALL_S = a.stall_s
        run_all(configs, specs, a.n, a.seed, a.out, a.scratch, metrics, a.max_retries,
                a.limit, a.force_plan, parity_fraction=a.parity_fraction,
                use_task_trials=not a.ignore_task_trials)
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
    if a.parity_fraction > 0:
        from analysis import parity

        res = parity.parity_check(parity.load_records(a.out))
        (a.out / "parity.json").write_text(json.dumps(res, indent=2))
        (a.out / "parity.md").write_text(parity.to_markdown(res))
        print(parity.to_markdown(res))
    return 0


if __name__ == "__main__":
    sys.exit(main())
