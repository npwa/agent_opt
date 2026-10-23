"""
analysis/compare_runs.py

Compare two run directories that were produced by the same runner and task set but with a different
setup (e.g. before/after a prompt change): pass rates, time per trial, time breakdown, and how much
of the first-call prefill is served from cache. Uses the shared statistics module (Wilson / Newcombe,
bootstrap, Mann-Whitney); numbers come from each run's ``summary.json`` and proxy records.

    python -m analysis.compare_runs --a results/v1/baseline --b results/v1/baseline_isolated
"""

import argparse
import json
import statistics as st
from pathlib import Path

from analysis import stats
from harness import promptbudget as pb


def _pct(v: float) -> str:
    return f"{v * 100:.0f} %"


def _ci(lo_hi, fmt) -> str:
    return f"[{fmt(lo_hi[0])}, {fmt(lo_hi[1])}]"


def compare(a_dir: Path, b_dir: Path) -> str:
    sa, sb = (json.loads((d / "summary.json").read_text()) for d in (a_dir, b_dir))
    ca, cb = sa["configs"][0], sb["configs"][0]
    tasks = [t for t in sa["tasks"] if f"{cb}|{t}" in sb["cells"]]
    cells_a = {t: sa["cells"][f"{ca}|{t}"] for t in tasks}
    cells_b = {t: sb["cells"][f"{cb}|{t}"] for t in tasks}
    L = [f"# Comparison: {a_dir.name} (A) vs {b_dir.name} (B)", "",
         f"{len(tasks)} common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), "
         "percentile bootstrap (times); the number of trials per task is in the pass counts (A pass, B pass).", ""]

    ka, na = sum(c["passes"] for c in cells_a.values()), sum(c["n"] for c in cells_a.values())
    kb, nb = sum(c["passes"] for c in cells_b.values()), sum(c["n"] for c in cells_b.values())
    r = stats.compare_rates(ka, na, kb, nb)
    L += ["## Success", "",
          f"- Overall: A {ka}/{na} = {_pct(r['rate_baseline'])} {_ci(r['wilson_baseline'], _pct)}; "
          f"B {kb}/{nb} = {_pct(r['rate_variant'])} {_ci(r['wilson_variant'], _pct)}; "
          f"difference {r['diff'] * 100:+.1f} pp {_ci(r['diff_ci'], lambda v: f'{v * 100:+.1f}')}.", "",
          "| Task | A pass | B pass | difference (95 % CI), pp |", "|---|---:|---:|---|"]
    for t in tasks:
        d = stats.compare_rates(cells_a[t]["passes"], cells_a[t]["n"], cells_b[t]["passes"], cells_b[t]["n"])
        L.append(f"| {t} | {cells_a[t]['passes']}/{cells_a[t]['n']} | {cells_b[t]['passes']}/{cells_b[t]['n']} | "
                 f"{d['diff'] * 100:+.0f} {_ci(d['diff_ci'], lambda v: f'{v * 100:+.0f}')} |")

    # time: per task median ratio, and pooled over tasks after normalising by A's median
    L += ["", "## Time per trial", "",
          "| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |", "|---|---:|---:|---|---:|"]
    pooled_a, pooled_b = [], []
    for t in tasks:
        xa, xb = cells_a[t]["times"], cells_b[t]["times"]
        c = stats.compare_samples(xa, xb)
        m = st.median(xa)
        pooled_a += [x / m for x in xa]
        pooled_b += [x / m for x in xb]
        L.append(f"| {t} | {st.median(xa):.1f} | {st.median(xb):.1f} | {c['ratio_median']:.2f} "
                 f"{_ci(c['ratio_ci'], lambda v: f'{v:.2f}')} | {c['p']:.3g} |")
    c = stats.compare_samples(pooled_a, pooled_b)
    mean_a = sum(sum(cells_a[t]["times"]) for t in tasks) / na
    mean_b = sum(sum(cells_b[t]["times"]) for t in tasks) / nb
    L += ["", f"- Pooled, normalised by each task's A median: median ratio B/A = {c['ratio_median']:.2f} "
          f"{_ci(c['ratio_ci'], lambda v: f'{v:.2f}')} (n = {len(pooled_a)} vs {len(pooled_b)}, Mann-Whitney p = {c['p']:.3g}).",
          f"- Mean time per trial: A {mean_a:.0f} s, B {mean_b:.0f} s ({(mean_b / mean_a - 1) * 100:+.0f} %). "
          f"Suite time (sum of task medians): A {sa['overview'][ca]['suite_s']:.0f} s, B {sb['overview'][cb]['suite_s']:.0f} s."]

    # time breakdown
    da, db = sa["decomposition"][ca]["all"]["pct"], sb["decomposition"][cb]["all"]["pct"]
    L += ["", "## Where the time goes (all tasks, share of wall-clock)", "", "| Bucket | A | B |", "|---|---:|---:|"]
    for k in ("prefill", "decode", "tool", "harness", "unattributed"):
        L.append(f"| {k} | {da[k]:.1f} % | {db[k]:.1f} % |")
    L.append(f"| wall-clock per trial | {sa['decomposition'][ca]['all']['wall_s'] / sa['decomposition'][ca]['all']['n']:.0f} s | "
             f"{sb['decomposition'][cb]['all']['wall_s'] / sb['decomposition'][cb]['all']['n']:.0f} s |")

    # prompt / cache
    pa, pbm = (pb.call_model(pb.load_calls(d / "proxy" / "metrics.jsonl")) for d in (a_dir, b_dir))
    ra, rb = (pb.cross_trial_reuse(pb.load_calls(d / "proxy" / "metrics.jsonl")) for d in (a_dir, b_dir))
    L += ["", "## Prompt and cache", "", "| | A | B |", "|---|---:|---:|",
          f"| first-call prompt tokens (median) | {pa['first_call_prompt_tokens_median']:.0f} | {pbm['first_call_prompt_tokens_median']:.0f} |",
          f"| cold prefill rate (tok/s) | {pa['cold_rate_tok_s']:.0f} | {pbm['cold_rate_tok_s']:.0f} |",
          f"| first-call ttft, median (s) | {pa['first_call_ttft_s']['median']:.1f} | {pbm['first_call_ttft_s']['median']:.1f} |",
          f"| first calls cold / warm | {pa['first_calls_cold']} / {pa['first_calls_warm']} | {pbm['first_calls_cold']} / {pbm['first_calls_warm']} |",
          f"| first calls, share of prefill time | {pa['first_call_share_of_prefill'] * 100:.0f} % | {pbm['first_call_share_of_prefill'] * 100:.0f} % |"]
    # order dependence: is a first call cold after a trial with a different prompt?
    calls = pb.load_calls(b_dir / "proxy" / "metrics.jsonl")
    rate = pbm["cold_rate_tok_s"]
    firsts = sorted((v[0] for v in calls.values() if v), key=lambda c: c["wall_start"])
    rows = []
    for prev, cur in zip(firsts, firsts[1:], strict=False):
        cold = cur["ttft_s"] >= 0.85 * cur["prompt_tokens"] / rate
        rows.append((cold, abs(cur["prompt_tokens"] - prev["prompt_tokens"]) <= 30, cur["task"], prev["task"]))
    L += ["", f"### Order dependence in B (first call of a trial vs the previous trial, {len(rows)} pairs)", "",
          "| previous trial's first prompt | first call cold | first call warm |", "|---|---:|---:|"]
    for similar, label in ((True, "same size (within 30 tokens)"), (False, "different size")):
        sel = [r for r in rows if r[1] == similar]
        L.append(f"| {label} | {sum(r[0] for r in sel)} | {sum(not r[0] for r in sel)} |")
    cold_rows = [r for r in rows if r[0]]
    if cold_rows:
        L += ["", "Cold first calls (task, previous task): " + "; ".join(f"{t} after {p}" for _, _, t, p in cold_rows) + "."]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare two run directories")
    ap.add_argument("--a", type=Path, required=True)
    ap.add_argument("--b", type=Path, required=True)
    ap.add_argument("--out", type=Path, help="default: <b>/COMPARISON.md")
    args = ap.parse_args()
    text = compare(args.a, args.b)
    out = args.out or args.b / "COMPARISON.md"
    out.write_text(text)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
