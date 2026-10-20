"""
analysis/report.py

Report generator (F14): one run directory -> RESULTS.md + report.html (+ summary.json).

    python -m analysis.report --run results/v1/baseline [--baseline CONFIG_ID] [--out DIR]

Everything is computed from files in the run directory (``plan.json``, ``platform.yaml``,
``trials/*/record.json``, optional ``trials/*/trace.json``, ``gpu_samples.csv``, ``parity.json``),
so a recorded run can be re-analysed with no GPU and no model, and the output is deterministic
(seeded bootstrap, fixed number formats, sorted iteration, no timestamps) - the replay test
compares it byte for byte.

Definitions
* a trial is *measured* if it is an instrumented trial that ended pass / fail / timeout;
  harness_failed trials are listed separately and excluded from every statistic
* success = pass (a timeout counts as a failure)
* time = wall-clock seconds of the trial
* comparisons vs the baseline pool the baseline-normalised times (t / baseline median of the
  same task) within a task category; p-values come from Mann-Whitney U and are Holm-adjusted over
  all (configuration x category) comparisons; a lever is "positive" only if the CI of the time
  change excludes 0 in the improving direction and success has not dropped by more than 5 pp
  (plan 7.4), "negative" if time got significantly worse or an established >5 pp success drop.
"""

import argparse
import json
import zlib
from pathlib import Path

import yaml

from analysis import stats, svg
from harness import decompose
from harness.taskspec import load_all
from harness.telemetry import GpuSamples

MEASURED = {"pass", "fail", "timeout"}
SUCCESS_DROP_LIMIT = 0.05
BOOT = 10_000


def _seed(*parts: str) -> int:
    return zlib.crc32("|".join(parts).encode())


# ---- loading --------------------------------------------------------------------------------------

def load_run(run_dir: Path, tasks_dir: Path) -> dict:
    run_dir = Path(run_dir)
    plan = json.loads((run_dir / "plan.json").read_text())
    platform = {}
    if (run_dir / "platform.yaml").exists():
        platform = yaml.safe_load((run_dir / "platform.yaml").read_text()) or {}
    parity = None
    if (run_dir / "parity.json").exists():
        parity = json.loads((run_dir / "parity.json").read_text())
    records, parity_n = [], 0
    for p in sorted((run_dir / "trials").glob("*/record.json")):
        r = json.loads(p.read_text())
        r["_trace"] = None
        tp = p.parent / "trace.json"
        if tp.exists():
            r["_trace"] = json.loads(tp.read_text())
        if r.get("instrumented", True):
            records.append(r)
        else:
            parity_n += 1
    samples = GpuSamples.from_csv(run_dir / "gpu_samples.csv")
    categories = {t.id: t.category for t in load_all(tasks_dir)} if Path(tasks_dir).exists() else {}
    return {"name": run_dir.name, "plan": plan, "platform": platform, "parity": parity,
            "records": records, "parity_trials": parity_n, "samples": samples, "categories": categories}


# ---- summary ---------------------------------------------------------------------------------------------

def _cell(recs: list[dict], key: str) -> dict:
    measured = [r for r in recs if r["status"] in MEASURED]
    times = [r["elapsed_s"] for r in measured if r.get("elapsed_s") is not None]
    n = len(measured)
    k = sum(r["status"] == "pass" for r in measured)
    out = {"n": n, "passes": k, "timeouts": sum(r["status"] == "timeout" for r in measured),
           "harness_failed": sum(r["status"] == "harness_failed" for r in recs),
           "rate": k / n if n else None, "wilson": list(stats.wilson_interval(k, n)) if n else None,
           "times": times}
    if times:
        out["median_s"] = stats.median(times)
        lo, hi = stats.bootstrap_ci(times, stats.median, resamples=BOOT, seed=_seed("cell", key))
        out["median_ci"] = [lo, hi]
    return out


def summarize(data: dict, baseline: str | None = None) -> dict:
    plan, records = data["plan"], data["records"]
    configs = list(plan["configs"])
    baseline = baseline or configs[0]
    if baseline not in configs:
        raise ValueError(f"baseline {baseline!r} not in run configs {configs}")
    tasks = sorted({r["task"] for r in records})
    cat = data["categories"]

    cells = {}
    for c in configs:
        for t in tasks:
            recs = [r for r in records if r["config"] == c and r["task"] == t]
            if recs:
                cells[(c, t)] = _cell(recs, f"{c}|{t}")

    common = [t for t in tasks if all((c, t) in cells and cells[(c, t)]["n"] for c in configs)]
    overview = {}
    for c in configs:
        recs = [r for r in records if r["config"] == c]
        meas = [r for r in recs if r["status"] in MEASURED]
        k = sum(r["status"] == "pass" for r in meas)
        in_common = [r for r in meas if r["task"] in common]
        kc = sum(r["status"] == "pass" for r in in_common)
        med = [cells[(c, t)] for t in common]
        overview[c] = {
            "n": len(meas), "passes": k, "wilson": list(stats.wilson_interval(k, len(meas))) if meas else None,
            "harness_failed": sum(r["status"] == "harness_failed" for r in recs),
            "timeouts": sum(r["status"] == "timeout" for r in meas),
            "n_common": len(in_common), "passes_common": kc,
            "rate_common": kc / len(in_common) if in_common else None,
            "wilson_common": list(stats.wilson_interval(kc, len(in_common))) if in_common else None,
            "suite_s": sum(m["median_s"] for m in med) if med else None,
            "suite_lo": sum(m["median_ci"][0] for m in med) if med else None,
            "suite_hi": sum(m["median_ci"][1] for m in med) if med else None,
        }

    # time decomposition (needs traces)
    decomps = []
    for r in records:
        if r["_trace"] and r["status"] in MEASURED:
            decomps.append(decompose.decompose(r["_trace"], data["samples"] if data["samples"].rows else None))
    decomp_by_cfg = {}
    for c in configs:
        ds = [d for d in decomps if d.get("config") == c and d.get("buckets_s")]
        if ds:
            agg = decompose.aggregate(ds, cat)
            allw = sum(d["wall_s"] for d in ds if d.get("buckets_s"))
            tot = {b: 100.0 * sum(d["buckets_s"][b] for d in ds if d.get("buckets_s")) / allw
                   for b in decompose.BUCKETS} if allw else None
            gpu = [d["gpu"] for d in ds if d.get("gpu") and d["gpu"].get("mean_util_pct") is not None]
            decomp_by_cfg[c] = {
                "categories": agg, "all": {"n": sum(a["n"] for a in agg.values()), "wall_s": allw, "pct": tot},
                "flagged": [d["trial_id"] for d in ds if not d.get("ok", False)],
                "gpu_duty_pct": sum(g["duty_cycle_pct"] for g in gpu) / len(gpu) if gpu else None,
                "first_call_share_pct": _mean([d["first_call"]["share_of_prefill_pct"] for d in ds if d.get("first_call")]),
            }

    comparisons = _compare(records, configs, baseline, cat)
    return {
        "name": data["name"], "created_utc": plan.get("created_utc"), "run_seed": plan.get("run_seed"),
        "n": plan.get("n"), "parity_fraction": plan.get("parity_fraction", 0.0),
        "configs": configs, "baseline": baseline, "tasks": tasks, "common_tasks": common,
        "categories": {t: cat.get(t, "unknown") for t in tasks},
        "platform": _platform(data["platform"]), "cells": {f"{c}|{t}": v for (c, t), v in cells.items()},
        "overview": overview, "decomposition": decomp_by_cfg, "comparisons": comparisons,
        "parity": data["parity"], "parity_trials": data["parity_trials"],
        "trials": {"measured": sum(o["n"] for o in overview.values()),
                   "harness_failed": sum(o["harness_failed"] for o in overview.values())},
    }


def _mean(xs: list) -> float | None:
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else None


def _platform(p: dict) -> dict:
    gpu = p.get("gpu", {})
    return {"gpu": gpu.get("name"), "driver": gpu.get("driver"), "cuda": gpu.get("cuda"),
            "ollama": p.get("ollama"), "opencode": p.get("opencode"),
            "llama_cpp": (p.get("llama_cpp") or {}).get("commit"), "python": p.get("python")}


def _compare(records: list[dict], configs: list[str], baseline: str, cat: dict) -> list[dict]:
    meas = [r for r in records if r["status"] in MEASURED]
    by = {}
    for r in meas:
        by.setdefault((r["config"], r["task"]), []).append(r)
    base_med = {t: stats.median([r["elapsed_s"] for r in rs]) for (c, t), rs in by.items() if c == baseline}
    rows = []
    for c in configs:
        if c == baseline:
            continue
        for category in sorted({cat.get(t, "unknown") for (_, t) in by}):
            tasks = sorted(t for t in base_med if cat.get(t, "unknown") == category and (c, t) in by)
            if not tasks:
                continue
            nb = [r["elapsed_s"] / base_med[t] for t in tasks for r in by[(baseline, t)]]
            nv = [r["elapsed_s"] / base_med[t] for t in tasks for r in by[(c, t)]]
            kb = sum(r["status"] == "pass" for t in tasks for r in by[(baseline, t)])
            kv = sum(r["status"] == "pass" for t in tasks for r in by[(c, t)])
            tcmp = stats.compare_samples(nb, nv, resamples=BOOT, seed=_seed("cmp", c, category))
            rcmp = stats.compare_rates(kb, len(nb), kv, len(nv))
            rows.append({"config": c, "category": category, "tasks": tasks, "time": tcmp, "success": rcmp})
    adj = stats.holm_adjust([r["time"]["p"] for r in rows])
    for r, a in zip(rows, adj, strict=True):
        r["p_holm"] = a
        t, s = r["time"], r["success"]
        if t["improved"] and s["diff"] >= -SUCCESS_DROP_LIMIT:
            r["verdict"] = "positive"
        elif (t["delta_ci"][0] > 0) or (s["diff_ci"][1] < -SUCCESS_DROP_LIMIT):
            r["verdict"] = "negative"
        else:
            r["verdict"] = "inconclusive"
    return rows


# ---- formatting helpers ---------------------------------------------------------------------------------------

def pct(v: float | None, d: int = 0) -> str:
    return "n/a" if v is None else f"{100 * v:.{d}f}%"


def sec(v: float | None, d: int = 1) -> str:
    return "n/a" if v is None else f"{v:.{d}f}"


def ci(lo_hi: list | None, fmt) -> str:
    return "" if not lo_hi else f"[{fmt(lo_hi[0])}, {fmt(lo_hi[1])}]"


def p1(v: float) -> str:
    """One-decimal percent value without a negative zero."""
    t = f"{v:.1f}"
    return t[1:] if t == "-0.0" else t


# ---- Markdown -----------------------------------------------------------------------------------------------------------

def render_markdown(s: dict) -> str:
    L = [f"# Results: {s['name']}", "",
         f"Run seed {s['run_seed']}, n = {s['n']} per cell, configurations: {', '.join(s['configs'])} "
         f"(baseline: **{s['baseline']}**). {s['trials']['measured']} measured trials, "
         f"{s['trials']['harness_failed']} harness failures excluded"
         + (f", {s['parity_trials']} uninstrumented parity trials (see parity)" if s['parity_trials'] else "")
         + ". Charts: `report.html`.", ""]
    p = s["platform"]
    L += [f"Platform: {p['gpu']} (driver {p['driver']}, CUDA {p['cuda']}), Ollama {p['ollama']}, "
          f"OpenCode {p['opencode']}, llama.cpp {p['llama_cpp']}, Python {p['python']}.", ""]
    L += ["## Configurations", "",
          "| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | "
          "suite time, sum of task medians (s) |", "|---|---:|---:|---:|---:|---:|"]
    for c in s["configs"]:
        o = s["overview"][c]
        rate = pct(o["passes"] / o["n"]) if o["n"] else "n/a"
        L.append(f"| {c} | {o['n']} | {rate} {ci(o['wilson'], pct)} | {o['timeouts']} | {o['harness_failed']} "
                 f"| {sec(o['suite_s'])} {ci([o['suite_lo'], o['suite_hi']] if o['suite_s'] else None, sec)} |")
    L += ["", "## Per task", "",
          "| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |",
          "|---|---|---:|---:|---|---|"]
    for t in s["tasks"]:
        for c in s["configs"]:
            cell = s["cells"].get(f"{c}|{t}")
            if cell and cell["n"]:
                L.append(f"| {t} | {c} | {cell['n']}{'*' if stats.is_directional(cell['n']) else ''} | "
                         f"{cell['passes']} | {pct(cell['rate'])} {ci(cell['wilson'], pct)} | "
                         f"{sec(cell['median_s'])} {ci(cell['median_ci'], sec)} |")
    L += ["", "\\* directional: fewer than 5 trials", ""]
    L += ["## Where the time goes", ""]
    if not s["decomposition"]:
        L += ["No traces in this run (run `python -m harness.trace --run <dir>` first).", ""]
    for c, d in s["decomposition"].items():
        L += [f"### {c}", "", "| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |",
              "|---|---:|---:|---:|---:|---:|---:|---:|"]
        for name, a in [*d["categories"].items(), ("all tasks", d["all"])]:
            q = a["pct"]
            L.append(f"| {name} | {a['n']} | {a['wall_s']:.0f} | {p1(q['prefill'])} | {p1(q['decode'])} | "
                     f"{p1(q['tool'])} | {p1(q['harness'])} | {p1(q['unattributed'])} |")
        extra = [f"NVML GPU activity (any kernel per sample window): {sec(d['gpu_duty_pct'], 0)}% of wall-clock"
                 if d["gpu_duty_pct"] is not None else None,
                 f"first LLM call of a session = {sec(d['first_call_share_pct'], 0)}% of prefill time"
                 if d["first_call_share_pct"] is not None else None,
                 f"{len(d['flagged'])} trial(s) flagged by the decomposition checks" if d["flagged"] else None]
        L += ["", "; ".join(e for e in extra if e) + ".", ""]
    L += ["## Comparison with the baseline", ""]
    if not s["comparisons"]:
        L += ["Only one configuration: nothing to compare.", ""]
    else:
        L += ["Times are normalised by the baseline median of the same task and pooled per category; "
              "p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).", "",
              "| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | "
              "success change (pp) [CI] | verdict |", "|---|---|---|---|---:|---:|---|---|"]
        for r in s["comparisons"]:
            t, q = r["time"], r["success"]
            L.append(f"| {r['config']} | {r['category']} | {t['n_baseline']}/{t['n_variant']}{'*' if t['directional'] else ''} | "
                     f"{t['ratio_median']:.2f} {ci(t['ratio_ci'], lambda v: f'{v:.2f}')} | {r['p_holm']:.3f} | "
                     f"{t['effect_size']['rank_biserial']:+.2f} | {100 * q['diff']:+.0f} "
                     f"{ci(q['diff_ci'], lambda v: f'{100 * v:+.0f}')} | {r['verdict']} |")
        L += ["", "\\* directional: fewer than 5 trials in a group", ""]
    if s["parity"]:
        pr = s["parity"]
        L += ["## Instrumentation parity", "", f"Verdict: **{pr['verdict']}** "
              f"({pr['n_parity_trials']} uninstrumented trials). See `parity.md`.", ""]
    return "\n".join(L) + "\n"


# ---- HTML ------------------------------------------------------------------------------------------------------------------

CSS = """
:root{color-scheme:light;--surface:#fcfcfb;--page:#f9f9f7;--ink:#0b0b0b;--ink2:#52514e;--muted:#898781;
--grid:#e1e0d9;--axis:#c3c2b7;--ring:rgba(11,11,11,.10);--good:#006300;
--series-1:#2a78d6;--series-2:#eb6834;--series-3:#1baf7a;--series-4:#eda100;--series-5:#e87ba4;--series-6:#008300;
--series-7:#6250d6;--series-8:#e34948;--series-x:#a9a8a1;--on-1:#fff;--on-x:#0b0b0b}
@media (prefers-color-scheme:dark){:root:where(:not([data-theme="light"])){color-scheme:dark;--surface:#1a1a19;--page:#0d0d0d;
--ink:#f0efec;--ink2:#c3c2b7;--grid:#2c2c2a;--axis:#383835;--ring:rgba(255,255,255,.10);--good:#0ca30c;
--series-1:#3987e5;--series-2:#d95926;--series-3:#199e70;--series-4:#c98500;--series-5:#d55181;--series-6:#008300;
--series-7:#9085e9;--series-8:#e66767;--series-x:#5d5c57;--on-1:#0b0b0b}}
:root[data-theme="dark"]{color-scheme:dark;--surface:#1a1a19;--page:#0d0d0d;--ink:#f0efec;--ink2:#c3c2b7;--grid:#2c2c2a;
--axis:#383835;--ring:rgba(255,255,255,.10);--good:#0ca30c;--series-1:#3987e5;--series-2:#d95926;--series-3:#199e70;
--series-4:#c98500;--series-5:#d55181;--series-6:#008300;--series-7:#9085e9;--series-8:#e66767;--series-x:#5d5c57;--on-1:#0b0b0b}
body{margin:0;background:var(--page);color:var(--ink);font:14px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:1040px;margin:0 auto;padding:24px 16px 48px}
h1{font-size:24px;margin:0 0 4px}h2{font-size:18px;margin:36px 0 8px}h3{font-size:15px;margin:20px 0 6px}
.sub{color:var(--ink2);margin:0 0 16px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin:16px 0}
.tile{background:var(--surface);border-radius:8px;padding:12px 16px;box-shadow:0 0 0 1px var(--ring)}
.tile .l{color:var(--ink2);font-size:12px}.tile .v{font-size:26px;font-weight:600;margin-top:2px}.tile .d{color:var(--ink2);font-size:12px}
figure{margin:12px 0;background:var(--surface);border-radius:8px;padding:12px 16px;box-shadow:0 0 0 1px var(--ring)}
figcaption{color:var(--ink2);font-size:12px;margin-top:6px}
.legend{display:flex;flex-wrap:wrap;gap:4px 16px;margin:0 0 8px;color:var(--ink2);font-size:12px}
.legend span.sw{display:inline-block;width:12px;height:12px;border-radius:3px;margin-right:6px;vertical-align:-2px}
.legend span.ln{display:inline-block;width:16px;height:0;border-top:2px solid var(--series-1);margin-right:6px;vertical-align:3px}
.sw.s1{background:var(--series-1)}.sw.s2{background:var(--series-2)}.sw.s3{background:var(--series-3)}
.sw.s4{background:var(--series-4)}.sw.s5{background:var(--series-5)}.sw.s6{background:var(--series-6)}
.sw.s7{background:var(--series-7)}.sw.s8{background:var(--series-8)}.sw.sx{background:var(--series-x)}
.sw.dom{background:var(--muted);border-radius:50%}.sw.round{border-radius:50%}.sw.s1.front{border-radius:50%}
table{border-collapse:collapse;width:100%;font-size:13px;margin:8px 0}
th,td{padding:5px 8px;border-bottom:1px solid var(--grid);text-align:right;font-variant-numeric:tabular-nums}
th:first-child,td:first-child,th.t,td.t{text-align:left}th{color:var(--ink2);font-weight:600}
details{margin:6px 0}summary{cursor:pointer;color:var(--ink2);font-size:12px}
.verdict-positive{color:var(--good);font-weight:600}.verdict-negative{font-weight:600}
svg.chart{display:block;max-width:100%;height:auto}svg text{font:11px system-ui,-apple-system,"Segoe UI",sans-serif;fill:var(--ink2)}
svg .tick{fill:var(--muted)}svg .rowlabel{fill:var(--ink)}svg .axislabel{fill:var(--ink2)}svg .pointlabel{fill:var(--ink)}
svg .grid{stroke:var(--grid);stroke-width:1}svg .axis{stroke:var(--axis);stroke-width:1}
svg .s1{fill:var(--series-1);stroke:var(--series-1)}svg .s2{fill:var(--series-2);stroke:var(--series-2)}
svg .s3{fill:var(--series-3);stroke:var(--series-3)}svg .s4{fill:var(--series-4);stroke:var(--series-4)}
svg .s5{fill:var(--series-5);stroke:var(--series-5)}svg .s6{fill:var(--series-6);stroke:var(--series-6)}
svg .s7{fill:var(--series-7);stroke:var(--series-7)}svg .s8{fill:var(--series-8);stroke:var(--series-8)}
svg .sx{fill:var(--series-x);stroke:none}svg .seg{stroke:none}
svg .whisker{stroke-width:2;fill:none}svg .whisker2{stroke:var(--muted);stroke-width:1.5;fill:none}
svg .dot{stroke:var(--surface);stroke-width:2}svg .pt.s1 .dot,svg .pt.s2 .dot,svg .pt.s3 .dot,svg .pt.s4 .dot,
svg .pt.s5 .dot,svg .pt.s6 .dot,svg .pt.s7 .dot,svg .pt.s8 .dot{stroke:var(--surface)}
svg .pt.s1 .whisker,svg .pt.s2 .whisker,svg .pt.s3 .whisker,svg .pt.s4 .whisker,svg .pt.s5 .whisker,
svg .pt.s6 .whisker,svg .pt.s7 .whisker,svg .pt.s8 .whisker{fill:none}
svg .pt.front .dot{fill:var(--series-1)}svg .pt.dominated .dot{fill:var(--muted)}
svg .frontier{fill:none;stroke:var(--series-1);stroke-width:2;stroke-linejoin:round}
svg .seglabel{fill:var(--ink);font-weight:600}svg .seglabel.on-s1{fill:var(--on-1)}
svg .pt:hover .dot,svg .seg:hover{opacity:.82}
@media (forced-colors:active){svg .seg,svg .dot{forced-color-adjust:none}}
"""

SEG_LABEL = {"prefill": "Prefill (time to first token)", "decode": "Decode", "tool": "Tool execution",
             "harness": "Harness / idle", "unattributed": "Unattributed"}


def _legend(items: list[tuple[str, str]], round_: bool = False) -> str:
    extra = " round" if round_ else ""
    return '<div class="legend">' + "".join(
        f'<span><span class="sw {cls}{extra}"></span>{svg.esc(name)}</span>' for cls, name in items) + "</div>"


def _table(headers: list[str], rows: list[list[str]], first_text: int = 1) -> str:
    th = "".join(f'<th class="t">{svg.esc(h)}</th>' if i < first_text else f"<th>{svg.esc(h)}</th>"
                 for i, h in enumerate(headers))
    body = "".join("<tr>" + "".join(
        (f'<td class="t">{c}</td>' if i < first_text else f"<td>{c}</td>") for i, c in enumerate(r)) + "</tr>"
        for r in rows)
    return f"<table><thead><tr>{th}</tr></thead><tbody>{body}</tbody></table>"


def render_html(s: dict) -> str:
    e = svg.esc
    o_all = s["overview"]
    measured = sum(o["n"] for o in o_all.values())
    passes = sum(o["passes"] for o in o_all.values())
    wl = stats.wilson_interval(passes, measured) if measured else (0, 1)
    first = next(iter(s["decomposition"].values()), None)
    dec = first["all"]["pct"] if first else None
    tiles = [("Measured trials", f"{measured}", f"{s['trials']['harness_failed']} harness failures excluded"),
             ("Success rate", pct(passes / measured) if measured else "n/a", f"95% CI {pct(wl[0])}-{pct(wl[1])}")]
    if dec:
        tiles += [("Decode share of wall-clock", f"{dec['decode']:.0f}%", "all tasks, first configuration"),
                  ("Prefill share of wall-clock", f"{dec['prefill']:.0f}%",
                   f"first LLM call = {sec(first['first_call_share_pct'], 0)}% of it")]
    h = [f'<!doctype html><html lang="en"><head><meta charset="utf-8">'
         f'<meta name="viewport" content="width=device-width,initial-scale=1">'
         f"<title>agent_opt results - {e(s['name'])}</title><style>{CSS}</style></head><body><main>",
         f"<h1>Results: {e(s['name'])}</h1>",
         f'<p class="sub">Run seed {e(s["run_seed"])} · n = {e(s["n"])} per cell · configurations: '
         f"{e(', '.join(s['configs']))} · baseline: {e(s['baseline'])}</p>",
         '<div class="tiles">' + "".join(
             f'<div class="tile"><div class="l">{e(a)}</div><div class="v">{e(b)}</div><div class="d">{e(c)}</div></div>'
             for a, b, c in tiles) + "</div>"]
    p = s["platform"]
    h.append(f'<p class="sub">{e(p["gpu"])} · driver {e(p["driver"])} · CUDA {e(p["cuda"])} · Ollama {e(p["ollama"])} · '
             f'OpenCode {e(p["opencode"])} · llama.cpp {e(p["llama_cpp"])}</p>')

    # configurations table
    rows = []
    for c in s["configs"]:
        o = o_all[c]
        rows.append([e(c), str(o["n"]), f"{pct(o['passes'] / o['n']) if o['n'] else 'n/a'} {e(ci(o['wilson'], pct))}",
                     str(o["timeouts"]), str(o["harness_failed"]),
                     f"{sec(o['suite_s'])} {e(ci([o['suite_lo'], o['suite_hi']] if o['suite_s'] else None, sec))}"])
    h += ["<h2>Configurations</h2>",
          _table(["Configuration", "trials", "pass rate (95% Wilson)", "timeouts", "harness failures",
                  "suite time, sum of task medians (s)"], rows)]

    # Pareto
    pts = []
    for c in s["configs"]:
        o = o_all[c]
        if o["rate_common"] is None or o["suite_s"] is None:
            continue
        pts.append({"label": c, "x": o["suite_s"], "y": o["rate_common"], "x_lo": o["suite_lo"], "x_hi": o["suite_hi"],
                    "y_lo": o["wilson_common"][0], "y_hi": o["wilson_common"][1],
                    "title": f"{c}: success {pct(o['rate_common'])} {ci(o['wilson_common'], pct)}, "
                             f"suite time {sec(o['suite_s'])} s {ci([o['suite_lo'], o['suite_hi']], sec)}"})
    if pts:
        h += ["<h2>Success versus time</h2>",
              "<figure>" + _legend([("s1", "On the Pareto front"), ("dom", "Dominated by another configuration")], round_=True)
              + svg.pareto(pts, label="Success rate versus suite time per configuration")
              + f"<figcaption>One point per configuration over the {len(s['common_tasks'])} tasks every "
                "configuration ran. x = sum of per-task median times (whiskers: sum of the per-task 95% "
                "bootstrap bounds); y = pooled pass rate (whiskers: 95% Wilson). Values are in the table above "
                "and in the tooltips.</figcaption></figure>"]

    # per-task dot + whisker charts
    n_cfg = len(s["configs"])
    legend = (_legend([(f"s{i + 1}", c) for i, c in enumerate(s["configs"])], round_=True)
              if n_cfg > 1 else "")
    for title, key, xmax in (("Success rate per task", "rate", 1.0), ("Time per task", "time", None)):
        tick_fmt = pct if key == "rate" else (lambda v: f"{v:g}")
        rws = []
        for t in s["tasks"]:
            ptsr = []
            for i, c in enumerate(s["configs"]):
                cell = s["cells"].get(f"{c}|{t}")
                if not cell or not cell["n"]:
                    continue
                if key == "rate":
                    v, lo, hi = cell["rate"], cell["wilson"][0], cell["wilson"][1]
                    ttl = f"{t} · {c}: {pct(v)} of {cell['n']} {ci(cell['wilson'], pct)}"
                else:
                    v, (lo, hi) = cell["median_s"], cell["median_ci"]
                    ttl = f"{t} · {c}: median {sec(v)} s of {cell['n']} {ci(cell['median_ci'], sec)}"
                ptsr.append({"series": i, "v": v, "lo": lo, "hi": hi, "title": ttl})
            rws.append({"label": t, "points": ptsr})
        h += [f"<h2>{e(title)}</h2>", "<figure>" + legend
              + svg.dot_whisker(rws, n_cfg, "pass rate (95% Wilson interval)" if key == "rate"
                                else "median wall-clock time per trial, s (95% bootstrap interval)",
                                tick_fmt, xmax, title)
              + "</figure>"]
        trs = []
        for t in s["tasks"]:
            for c in s["configs"]:
                cell = s["cells"].get(f"{c}|{t}")
                if cell and cell["n"]:
                    if key == "rate":
                        trs.append([e(t), e(c), f"{cell['n']}{'*' if stats.is_directional(cell['n']) else ''}",
                                    str(cell["passes"]), f"{pct(cell['rate'])} {e(ci(cell['wilson'], pct))}"])
                    else:
                        trs.append([e(t), e(c), str(cell["n"]), sec(cell["median_s"]),
                                    e(ci(cell["median_ci"], sec))])
        hdr = (["Task", "Configuration", "n", "pass", "rate (95% Wilson)"] if key == "rate"
               else ["Task", "Configuration", "n", "median s", "95% bootstrap CI"])
        h.append("<details><summary>Table view</summary>" + _table(hdr, trs, 2) + "</details>")

    # decomposition
    h.append("<h2>Where the time goes</h2>")
    if not s["decomposition"]:
        h.append("<p>No traces in this run (<code>python -m harness.trace --run &lt;dir&gt;</code>).</p>")
    for c, d in s["decomposition"].items():
        rws = []
        for name, a in [*d["categories"].items(), ("all tasks", d["all"])]:
            rws.append({"label": name, "total_text": f"n={a['n']} · {a['wall_s'] / max(1, a['n']):.0f} s/trial",
                        "segments": a["pct"],
                        "titles": {k: f"{name}: {SEG_LABEL[k]} {a['pct'][k]:.1f}% of {a['wall_s']:.0f} s" for k in a["pct"]}})
        trs = [[e(name), str(a["n"]), f"{a['wall_s']:.0f}"] + [p1(a['pct'][b]) for b in decompose.BUCKETS]
               for name, a in [*d["categories"].items(), ("all tasks", d["all"])]]
        notes = []
        if d["gpu_duty_pct"] is not None:
            notes.append(f"NVML reports the GPU as non-idle for {sec(d['gpu_duty_pct'], 0)}% of wall-clock "
                         "(any kernel in the sample window; real kernel-active time is far lower, see the Nsight pilot).")
        if d["flagged"]:
            notes.append(f"{len(d['flagged'])} trial(s) flagged by the decomposition checks: "
                         + e(", ".join(d["flagged"])) + ".")
        h += [f"<h3>{e(c)}</h3>", "<figure>"
              + _legend([(svg.SEGMENT_CLASS[k], SEG_LABEL[k]) for k in svg.SEGMENT_KEYS])
              + svg.stacked_bars(rws, f"Time decomposition, {c}")
              + "<figcaption>" + " ".join(notes) + "</figcaption></figure>",
              "<details><summary>Table view</summary>"
              + _table(["Category", "n", "wall s", "prefill %", "decode %", "tool %", "harness %", "unattributed %"], trs)
              + "</details>"]

    # comparisons
    h.append("<h2>Comparison with the baseline</h2>")
    if not s["comparisons"]:
        h.append("<p>Only one configuration: nothing to compare.</p>")
    else:
        rows = []
        for r in s["comparisons"]:
            t, q = r["time"], r["success"]
            vcls = f' class="verdict-{r["verdict"]}"' if r["verdict"] != "inconclusive" else ""
            mark = {"positive": "▲ ", "negative": "▼ "}.get(r["verdict"], "")
            rows.append([e(r["config"]), e(r["category"]),
                         f"{t['n_baseline']}/{t['n_variant']}{'*' if t['directional'] else ''}",
                         f"{t['ratio_median']:.2f} {e(ci(t['ratio_ci'], lambda v: f'{v:.2f}'))}",
                         f"{r['p_holm']:.3f}", f"{t['effect_size']['rank_biserial']:+.2f}",
                         f"{100 * q['diff']:+.0f} {e(ci(q['diff_ci'], lambda v: f'{100 * v:+.0f}'))}",
                         f"<span{vcls}>{mark}{r['verdict']}</span>"])
        h += [_table(["Configuration", "Category", "n base/var", "time ratio (median) [95% CI]", "p (Holm)",
                      "effect (rank-biserial)", "success change, pp [95% CI]", "verdict"], rows, 2),
              '<p class="sub">Times normalised by the baseline median of the same task, pooled per category. '
              "p: Mann-Whitney U, Holm-adjusted over this table. Positive = time CI excludes 0 in the improving "
              "direction and success not down by more than 5 pp; negative = significantly slower or an established "
              "&gt;5 pp success drop. * = fewer than 5 trials in a group (directional).</p>"]
    if s["parity"]:
        pr = s["parity"]
        h += ["<h2>Instrumentation parity</h2>",
              f"<p>Verdict: <strong>{e(pr['verdict'])}</strong> ({e(pr['n_parity_trials'])} uninstrumented "
              "trials compared with instrumented trials of the same cells).</p>"]
    h.append("</main></body></html>")
    return "".join(h) + "\n"


# ---- CLI ------------------------------------------------------------------------------------------------------------------------

def write_report(run_dir: Path, out_dir: Path | None = None, tasks_dir: Path = Path("tasks"),
                 baseline: str | None = None) -> dict:
    out_dir = Path(out_dir or run_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    s = summarize(load_run(run_dir, tasks_dir), baseline)
    (out_dir / "RESULTS.md").write_text(render_markdown(s))
    (out_dir / "report.html").write_text(render_html(s))
    (out_dir / "summary.json").write_text(json.dumps(s, indent=2, sort_keys=True, default=_json_default) + "\n")
    return s


def _json_default(o):
    if isinstance(o, tuple):
        return list(o)
    raise TypeError(type(o))


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate RESULTS.md + report.html for a run directory")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--baseline")
    ap.add_argument("--tasks", type=Path, default=Path("tasks"))
    a = ap.parse_args()
    s = write_report(a.run, a.out, a.tasks, a.baseline)
    print(f"report for {s['name']}: {s['trials']['measured']} measured trials -> {a.out or a.run}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
