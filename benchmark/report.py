#!/usr/bin/env python3
"""
report.py

Reads benchmark_results.csv (produced by join_metrics.py) and prints a short
per-task performance summary: call count, latency, throughput, and resource
utilization, averaged across all calls under each task label.

Run after join_metrics.py:
    python3 report.py
Writes: report.md (also prints to stdout)
"""

import csv
import statistics as stats
from collections import defaultdict
from pathlib import Path

BASE = Path(__file__).parent
IN_PATH = BASE / "benchmark_results.csv"
OUT_PATH = BASE / "report.md"


def to_float(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


def avg(values):
    values = [v for v in values if v is not None]
    return round(stats.mean(values), 2) if values else None


def main():
    if not IN_PATH.exists():
        print(f"{IN_PATH} not found — run join_metrics.py first.")
        return

    by_task = defaultdict(list)
    with open(IN_PATH) as f:
        for row in csv.DictReader(f):
            by_task[row["task"]].append(row)

    lines = ["# Benchmark performance summary", ""]

    for task, rows in sorted(by_task.items()):
        ttft = avg([to_float(r["ttft_s"]) for r in rows])
        total_s = avg([to_float(r["total_s"]) for r in rows])
        in_tok_s = avg([to_float(r["input_tok_s"]) for r in rows])
        out_tok_s = avg([to_float(r["output_tok_s"]) for r in rows])
        gpu = avg([to_float(r["avg_gpu_util_pct"]) for r in rows])
        cpu = avg([to_float(r["avg_cpu_pct"]) for r in rows])
        ram = avg([to_float(r["avg_ram_pct"]) for r in rows])
        completion_tokens = [to_float(r["completion_tokens"]) for r in rows]
        approx_flags = [r.get("completion_tokens_is_approx") for r in rows]
        approx_note = " (approx.)" if any(f in ("True", "true") for f in approx_flags) else ""

        lines.append(f"## {task}")
        lines.append("")
        lines.append(f"- Calls: {len(rows)}")
        lines.append(f"- Avg time to first token: {ttft} s")
        lines.append(f"- Avg total call time: {total_s} s")
        lines.append(f"- Avg input throughput: {in_tok_s} tok/s")
        lines.append(f"- Avg output throughput: {out_tok_s} tok/s")
        lines.append(f"- Avg completion tokens/call: {avg(completion_tokens)}{approx_note}")
        lines.append(f"- Avg GPU utilization during call: {gpu}%")
        lines.append(f"- Avg CPU utilization during call: {cpu}%")
        lines.append(f"- Avg RAM utilization during call: {ram}%")
        lines.append("")

    report = "\n".join(lines)
    with open(OUT_PATH, "w") as f:
        f.write(report)

    print(report)
    print(f"\n(written to {OUT_PATH})")


if __name__ == "__main__":
    main()
