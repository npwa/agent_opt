#!/usr/bin/env python3
"""
harness/join_metrics.py

Joins proxy records (metrics.jsonl) with 1 Hz resource samples (samples.csv):
each request gets the mean GPU/CPU/RAM utilisation of the samples that fall
inside [wall_start, wall_start + total_s]. Writes benchmark_results.csv, the
input of harness.report.

(The original v0 script was not committed; this reconstructs it from the
column contract that report.py consumes.)
"""

import csv
import json
import os
import statistics
from pathlib import Path

DATA_DIR = Path(os.environ.get("AGENT_OPT_DATA_DIR", Path.cwd()))

FIELDS = [
    "task", "wall_start", "ttft_s", "total_s", "prompt_tokens", "completion_tokens",
    "completion_tokens_is_approx", "input_tok_s", "output_tok_s",
    "avg_gpu_util_pct", "avg_cpu_pct", "avg_ram_pct",
]


def _f(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


def _mean(values):
    values = [v for v in values if v is not None]
    return round(statistics.mean(values), 2) if values else None


def load_samples(path: Path) -> list[dict]:
    with open(path) as f:
        return [
            {"t": float(r["wall_time"]), "gpu": _f(r["gpu_util_pct"]),
             "cpu": _f(r["cpu_pct"]), "ram": _f(r["ram_pct"])}
            for r in csv.DictReader(f)
        ]


def load_records(path: Path) -> list[dict]:
    with open(path) as f:
        return [json.loads(line) for line in f if line.strip()]


def join(records: list[dict], samples: list[dict]) -> list[dict]:
    rows = []
    for rec in records:
        t0 = rec["wall_start"]
        t1 = t0 + rec["total_s"]
        window = [s for s in samples if t0 <= s["t"] <= t1]
        row = {k: rec.get(k) for k in FIELDS}
        row["avg_gpu_util_pct"] = _mean([s["gpu"] for s in window])
        row["avg_cpu_pct"] = _mean([s["cpu"] for s in window])
        row["avg_ram_pct"] = _mean([s["ram"] for s in window])
        rows.append(row)
    return rows


def write_csv(rows: list[dict], path: Path) -> None:
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)


def main() -> None:
    rows = join(load_records(DATA_DIR / "metrics.jsonl"), load_samples(DATA_DIR / "samples.csv"))
    write_csv(rows, DATA_DIR / "benchmark_results.csv")
    print(f"{len(rows)} requests joined -> {DATA_DIR / 'benchmark_results.csv'}")


if __name__ == "__main__":
    main()
