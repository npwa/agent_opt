"""
analysis/consistency.py

How consistent are the outcomes of repeated trials? A lever that lowers the sampling noise (a lower temperature) should
push cells (task x configuration) towards all-pass or all-fail, even where it does not change the pass rate. Per
configuration: how many cells passed every time, failed every time, or were mixed, and the mean within-cell variance of the
pass indicator (p (1 - p) with p the cell's pass rate; 0 = fully consistent, 0.25 = a coin flip).

    python -m analysis.consistency --run results/v1/m5_temperature
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path


def analyse(run_dir: Path) -> dict:
    cells = defaultdict(list)
    for f in sorted(Path(run_dir, "trials").glob("*/record.json")):
        r = json.loads(f.read_text())
        if r.get("instrumented", True) and r.get("status") != "harness_failed":
            cells[(r["config"], r["task"])].append(1 if r.get("status") == "pass" else 0)
    out = defaultdict(lambda: {"cells": 0, "all_pass": 0, "all_fail": 0, "mixed": 0, "trials": 0, "passes": 0, "variance": []})
    for (cfg, _), v in cells.items():
        o, p = out[cfg], sum(v) / len(v)
        o["cells"] += 1
        o["trials"] += len(v)
        o["passes"] += sum(v)
        o["all_pass"] += p == 1.0
        o["all_fail"] += p == 0.0
        o["mixed"] += 0.0 < p < 1.0
        o["variance"].append(p * (1 - p))
    return {cfg: {**{k: v for k, v in o.items() if k != "variance"}, "mean_within_cell_variance": sum(o["variance"]) / len(o["variance"])}
            for cfg, o in sorted(out.items())}


def render(name: str, res: dict) -> str:
    L = [f"# Consistency of repeated trials: {name}", "",
         "| Configuration | cells | trials | pass rate | all pass | all fail | mixed | mean within-cell variance |", "|---|---:|---:|---:|---:|---:|---:|---:|"]
    for cfg, o in res.items():
        L.append(f"| {cfg} | {o['cells']} | {o['trials']} | {o['passes'] / o['trials'] * 100:.0f} % | {o['all_pass']} | {o['all_fail']} | "
                 f"{o['mixed']} | {o['mean_within_cell_variance']:.3f} |")
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Consistency of repeated trials per configuration")
    ap.add_argument("--run", type=Path, required=True)
    a = ap.parse_args()
    res = analyse(a.run)
    (a.run / "consistency.json").write_text(json.dumps(res, indent=2) + "\n")
    (a.run / "consistency.md").write_text(render(a.run.name, res))
    print(render(a.run.name, res))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
