"""
analysis/parity.py

Instrumentation parity check (F7): are results measured *through* the metrics proxy the same
as results from OpenCode talking to Ollama directly?

Pairs: for every (config, task) cell that has uninstrumented ("parity") trials, those trials are
compared with the instrumented trials of the same cell.

* time    : r = parity wall time / median instrumented wall time of its cell; pooled geometric
            mean ratio with a percentile-bootstrap 95 % CI.
              violation     CI lies entirely outside [0.95, 1.05]
              ok            CI lies entirely inside  [0.95, 1.05]
              inconclusive  otherwise (more parity trials needed)
* success : pooled pass rate of the instrumented trials of those cells vs the parity trials,
            Wilson 95 % intervals. violation = intervals do not overlap.

Trials ending as ``harness_failed`` are excluded (they carry no measurement); timeouts are kept.
"""

import json
import math
import statistics
import sys
from pathlib import Path

from analysis.stats import bootstrap_ci, geometric_mean, wilson_interval

TIME_TOLERANCE = 0.05
MIN_PARITY_TRIALS = 5
MEASURED = {"pass", "fail", "timeout"}


def load_records(run_dir: Path) -> list[dict]:
    out = []
    for p in sorted((Path(run_dir) / "trials").glob("*/record.json")):
        r = json.loads(p.read_text())
        if r.get("status") in MEASURED:
            out.append(r)
    return out


def parity_check(records: list[dict], seed: int = 0, resamples: int = 10_000) -> dict:
    cells: dict[tuple[str, str], dict[str, list[dict]]] = {}
    for r in records:
        key = (r["config"], r["task"])
        cells.setdefault(key, {"instr": [], "parity": []})[
            "instr" if r.get("instrumented", True) else "parity"].append(r)

    ratios, instr_pass, instr_n, par_pass, par_n, skipped = [], 0, 0, 0, 0, 0
    per_cell = []
    for (cfg, task), g in sorted(cells.items()):
        if not g["parity"]:
            continue
        if not g["instr"]:
            skipped += len(g["parity"])
            continue
        med = statistics.median(r["elapsed_s"] for r in g["instr"])
        cell_ratios = [r["elapsed_s"] / med for r in g["parity"] if r.get("elapsed_s") and med > 0]
        ratios += cell_ratios
        instr_pass += sum(r["status"] == "pass" for r in g["instr"])
        instr_n += len(g["instr"])
        par_pass += sum(r["status"] == "pass" for r in g["parity"])
        par_n += len(g["parity"])
        per_cell.append({"config": cfg, "task": task, "n_instrumented": len(g["instr"]),
                         "n_parity": len(g["parity"]), "median_instrumented_s": med,
                         "median_parity_s": statistics.median(r["elapsed_s"] for r in g["parity"])})

    res: dict = {"n_pairs": len(ratios), "n_parity_trials": par_n, "n_instrumented_in_cells": instr_n,
                 "parity_without_instrumented_partner": skipped, "cells": per_cell}
    if len(ratios) < MIN_PARITY_TRIALS:
        res["time"] = {"verdict": "insufficient", "reason": f"{len(ratios)} paired trials "
                       f"(need >= {MIN_PARITY_TRIALS})"}
        res["success"] = {"verdict": "insufficient", "reason": f"{par_n} parity trials"}
        res["verdict"] = "insufficient"
        return res

    logs = [math.log(r) for r in ratios]
    lo_l, hi_l = bootstrap_ci(logs, resamples=resamples, seed=seed)
    ratio, lo, hi = geometric_mean(ratios), math.exp(lo_l), math.exp(hi_l)
    if lo > 1 + TIME_TOLERANCE or hi < 1 - TIME_TOLERANCE:
        tv = "violation"
    elif lo >= 1 - TIME_TOLERANCE and hi <= 1 + TIME_TOLERANCE:
        tv = "ok"
    else:
        tv = "inconclusive"
    res["time"] = {"verdict": tv, "ratio": ratio, "ci": [lo, hi], "tolerance": TIME_TOLERANCE}

    ilo, ihi = wilson_interval(instr_pass, instr_n)
    plo, phi = wilson_interval(par_pass, par_n)
    overlap = max(ilo, plo) <= min(ihi, phi)
    res["success"] = {"verdict": "ok" if overlap else "violation",
                      "instrumented": {"rate": instr_pass / instr_n, "n": instr_n, "ci": [ilo, ihi]},
                      "parity": {"rate": par_pass / par_n, "n": par_n, "ci": [plo, phi]}}
    verdicts = {tv, res["success"]["verdict"]}
    res["verdict"] = ("violation" if "violation" in verdicts else
                      "ok" if verdicts == {"ok"} else "inconclusive")
    return res


def to_markdown(res: dict) -> str:
    t, s = res["time"], res["success"]
    lines = [f"# Instrumentation parity: **{res['verdict'].upper()}**", "",
             f"{res['n_parity_trials']} uninstrumented trials compared with "
             f"{res['n_instrumented_in_cells']} instrumented trials of the same cells.", ""]
    if t["verdict"] == "insufficient":
        lines.append(f"Not enough data: {t['reason']}.")
    else:
        lines += [f"- **Time**: {t['verdict']} - parity/instrumented ratio {t['ratio']:.3f} "
                  f"(95 % CI {t['ci'][0]:.3f}-{t['ci'][1]:.3f}, tolerance +/-{t['tolerance']:.0%}, "
                  f"n = {res['n_pairs']})",
                  f"- **Success rate**: {s['verdict']} - instrumented {s['instrumented']['rate']:.0%} "
                  f"(n = {s['instrumented']['n']}, CI {s['instrumented']['ci'][0]:.2f}-"
                  f"{s['instrumented']['ci'][1]:.2f}) vs uninstrumented {s['parity']['rate']:.0%} "
                  f"(n = {s['parity']['n']}, CI {s['parity']['ci'][0]:.2f}-{s['parity']['ci'][1]:.2f})"]
    if res["parity_without_instrumented_partner"]:
        lines.append(f"- {res['parity_without_instrumented_partner']} parity trials had no "
                     "instrumented partner and were ignored.")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    import argparse

    ap = argparse.ArgumentParser(description="Instrumentation parity check for a finished run")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--seed", type=int, default=0)
    a = ap.parse_args(argv)
    res = parity_check(load_records(a.run), seed=a.seed)
    (a.run / "parity.json").write_text(json.dumps(res, indent=2))
    md = to_markdown(res)
    (a.run / "parity.md").write_text(md)
    print(md)
    return 1 if res["verdict"] == "violation" else 0


if __name__ == "__main__":
    sys.exit(main())
