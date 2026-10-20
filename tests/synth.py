"""Deterministic synthetic run directory for report tests (clearly NOT real measurements)."""

import json
import random
from pathlib import Path

import yaml

TASKS = ["coding_02_fizzbuzz_gen", "coding_06_tax_rates_lan", "bugfix_01_moving_average", "repoqa_05_retry_policy"]
# config -> (time multiplier, prefill multiplier, success probability per task)
CONFIGS = {
    "baseline_32k": (1.0, 1.0, {"coding_02_fizzbuzz_gen": 0.9, "coding_06_tax_rates_lan": 0.4,
                                "bugfix_01_moving_average": 0.8, "repoqa_05_retry_policy": 0.9}),
    "trim_schemas": (0.7, 0.45, {"coding_02_fizzbuzz_gen": 0.9, "coding_06_tax_rates_lan": 0.4,
                                 "bugfix_01_moving_average": 0.8, "repoqa_05_retry_policy": 0.85}),
    "ctx_8k": (1.1, 1.05, {"coding_02_fizzbuzz_gen": 0.5, "coding_06_tax_rates_lan": 0.3,
                           "bugfix_01_moving_average": 0.3, "repoqa_05_retry_policy": 0.6}),
}
BASE_STEPS = {"coding_02_fizzbuzz_gen": 6, "coding_06_tax_rates_lan": 1, "bugfix_01_moving_average": 5,
              "repoqa_05_retry_policy": 3}


def make_trace(tid, task, config, status, rnd, tmult, pmult, t0):
    steps, t, n = [], t0 + 2.0, BASE_STEPS[task]
    for i in range(n):
        ttft = (38.0 * pmult if i == 0 else 2.5 * pmult) * rnd.uniform(0.9, 1.1)
        decode = rnd.uniform(4, 14) * tmult
        tool = rnd.uniform(0.02, 0.2) if i < n - 1 and task != "coding_06_tax_rates_lan" else 0.0
        llm = ttft + decode
        tools = [{"tool": "bash", "start": t + llm + 0.05, "end": t + llm + 0.05 + tool, "status": "completed"}] if tool else []
        steps.append({"step": i + 1, "t_start": t, "t_end": t + llm, "ttft_s": ttft, "llm_s": llm,
                      "prefill_tokens": 10000 + 300 * i, "decode_tokens": int(decode * 25), "tools": tools})
        t += llm + 0.3 + tool
    wall = t - t0 + 0.4
    return {"trial_id": tid, "task": task, "config": config, "status": status, "warnings": [],
            "unmatched_event_steps": [],
            "summary": {"wall_start": t0, "wall_end": t0 + wall, "wall_s": wall, "prefill_tokens": 10000 * n,
                        "decode_tokens": 300 * n}, "steps": steps}, wall


def make_run(root: Path, n: int = 8, seed: int = 3, configs=None, tasks=None, with_traces=True) -> Path:
    rnd = random.Random(seed)
    configs = configs or list(CONFIGS)
    tasks = tasks or TASKS
    root = Path(root)
    (root / "trials").mkdir(parents=True)
    (root / "plan.json").write_text(json.dumps({
        "created_utc": "2026-01-01T00:00:00Z", "run_seed": seed, "n": n, "parity_fraction": 0.0,
        "configs": {c: {"id": c, "model": "ollama/synthetic"} for c in configs}, "tasks": {t: "x" for t in tasks},
        "order": []}))
    (root / "platform.yaml").write_text(yaml.safe_dump({
        "gpu": {"name": "Synthetic GPU", "driver": "0", "cuda": "0"}, "ollama": "0.0", "opencode": "0.0",
        "llama_cpp": {"commit": "abc123"}, "python": "3.12"}))
    t0 = 1_000_000.0
    for c in configs:
        tmult, pmult, probs = CONFIGS[c]
        for task in tasks:
            for k in range(n):
                tid = f"{c}__{task}__{k:02d}"
                status = "pass" if rnd.random() < probs[task] else "fail"
                trace, wall = make_trace(tid, task, c, status, rnd, tmult, pmult, t0)
                t0 += wall + 5
                d = root / "trials" / tid
                d.mkdir()
                (d / "record.json").write_text(json.dumps({
                    "trial_id": tid, "task": task, "config": c, "k": k, "status": status, "elapsed_s": wall,
                    "instrumented": True, "attempts": [{"attempt": 1}]}))
                if with_traces:
                    (d / "trace.json").write_text(json.dumps(trace))
    return root
