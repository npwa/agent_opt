"""
harness/decompose.py

Time decomposition of an agent run (F6): where does wall-clock go?

    wall = prefill + decode + tool + harness + unattributed

* prefill   = sum of TTFT of the LLM calls (proxy). Includes model loading when the model was
              not resident and prompt processing that missed the prompt cache; see
              ``first_call`` for the share caused by the session's first call.
* decode    = sum of (call duration - TTFT) (proxy)
* tool      = time tools were running *inside the gaps between LLM calls* (union of the tool
              intervals from OpenCode events, so parallel tools are not double counted)
* harness   = time in those gaps (and before the first / after the last call) when no tool
              was running: OpenCode process start-up, request preparation, DB writes, MCP
              start-up and shutdown. Reported as ``startup``, ``between_calls`` and ``shutdown``.
* unattributed = wall - everything above. Because gaps are derived from the LLM windows this
              is zero for a clean join, and becomes non-zero exactly when the instrumentation
              is inconsistent: LLM windows overlapping each other or the run boundary
              (clock skew; also reported as ``llm_overlap_s`` and limited to 1 % of wall), or OpenCode steps that have no proxy record (their time would
              otherwise hide inside "harness"). The plan requires it to stay below 5 %;
              ``ok`` says whether it does.

Also reported: GPU duty cycle (share of wall-clock with utilisation above the idle threshold),
mean utilisation during prefill vs decode windows, and an energy estimate. These are NVML
numbers: NVML counts a sample window as busy if *any* kernel ran in it, so on this CPU-offloaded
model it overstates real GPU work by roughly 5x (Nsight Systems pilot: ~10 % GPU-active vs ~50 %
NVML utilisation, see results/v1/nsys_pilot). Treat them as "GPU not idle", not as busy time.
"""

import argparse
import json
import sys
from pathlib import Path

from harness.telemetry import GpuSamples

UNATTRIBUTED_LIMIT_PCT = 5.0
OVERLAP_LIMIT_PCT = 1.0
BUCKETS = ("prefill", "decode", "tool", "harness", "unattributed")


def merge(intervals: list[tuple[float, float]]) -> list[tuple[float, float]]:
    out: list[list[float]] = []
    for a, b in sorted(i for i in intervals if i[1] > i[0]):
        if out and a <= out[-1][1]:
            out[-1][1] = max(out[-1][1], b)
        else:
            out.append([a, b])
    return [(a, b) for a, b in out]


def overlap_len(merged: list[tuple[float, float]], a: float, b: float) -> float:
    return sum(max(0.0, min(b, y) - max(a, x)) for x, y in merged)


def _window_stats(samples: GpuSamples, windows: list[tuple[float, float]]) -> float | None:
    vals = []
    for a, b in windows:
        w = samples.window(a, b)
        if w.n and w.mean_util is not None:
            vals.append((w.mean_util, w.n))
    n = sum(c for _, c in vals)
    return sum(u * c for u, c in vals) / n if n else None


def decompose(trace: dict, samples: GpuSamples | None = None) -> dict:
    s = trace["summary"]
    wall0, wall1 = s["wall_start"], s["wall_end"]
    steps = trace["steps"]
    wall = wall1 - wall0
    out: dict = {"trial_id": trace["trial_id"], "task": trace.get("task"),
                 "config": trace.get("config"), "status": trace.get("status"), "wall_s": wall}
    if not steps or wall <= 0:
        out.update(ok=False, reason="no LLM calls joined" if not steps else "no wall-clock",
                   buckets_s={}, buckets_pct={})
        return out

    prefill = sum(st["ttft_s"] or 0.0 for st in steps)
    decode = sum(st["llm_s"] - (st["ttft_s"] or 0.0) for st in steps)
    tool_iv = merge([(t["start"], t["end"]) for st in steps for t in st["tools"]])

    # gaps: start-up, between calls, shutdown (negative = overlap, clamped to 0)
    startup = max(0.0, steps[0]["t_start"] - wall0)
    shutdown = max(0.0, wall1 - steps[-1]["t_end"])
    gaps = [(a["t_end"], b["t_start"]) for a, b in zip(steps, steps[1:], strict=False)]
    between_total = sum(max(0.0, b - a) for a, b in gaps)
    tool = sum(overlap_len(tool_iv, a, b) for a, b in gaps if b > a)
    tool += overlap_len(tool_iv, steps[-1]["t_end"], wall1)  # tools after the final call
    between = max(0.0, between_total - sum(overlap_len(tool_iv, a, b) for a, b in gaps if b > a))
    tail_tool = overlap_len(tool_iv, steps[-1]["t_end"], wall1)
    shutdown = max(0.0, shutdown - tail_tool)
    tool_total = sum(b - a for a, b in tool_iv)

    unmatched = sum(max(0.0, u["finish"] - u["start"]) for u in trace.get("unmatched_event_steps", []))
    between = max(0.0, between - unmatched)  # known LLM time that has no proxy record

    llm_overlap = sum(max(0.0, a["t_end"] - b["t_start"]) for a, b in zip(steps, steps[1:], strict=False))
    harness = startup + between + shutdown
    explained = prefill + decode + tool + harness
    unattributed = wall - explained
    pct = {k: 100.0 * v / wall for k, v in
           (("prefill", prefill), ("decode", decode), ("tool", tool), ("harness", harness),
            ("unattributed", unattributed))}
    out.update(
        buckets_s={"prefill": prefill, "decode": decode, "tool": tool, "harness": harness,
                   "unattributed": unattributed},
        buckets_pct=pct,
        harness_parts_s={"startup": startup, "between_calls": between, "shutdown": shutdown},
        tool_overlap_s=max(0.0, tool_total - tool),  # tool time outside gaps (e.g. inside LLM windows)
        llm_overlap_s=llm_overlap,  # LLM calls overlapping each other: sequential agents must have 0
        unmatched_event_s=unmatched,
        ok=(abs(pct["unattributed"]) < UNATTRIBUTED_LIMIT_PCT
            and 100.0 * llm_overlap / wall < OVERLAP_LIMIT_PCT and not trace.get("warnings")),
        warnings=trace.get("warnings", []),
        n_steps=len(steps),
        prefill_tokens=s["prefill_tokens"], decode_tokens=s["decode_tokens"],
    )
    # The first call of a session pays for the whole system prompt + tool schemas (and model
    # loading if the model was not resident); later calls reuse Ollama's prompt cache. Ollama's
    # OpenAI-compatible endpoint does not report load time, so the two cannot be separated -
    # the first call is reported on its own instead of guessing.
    first = steps[0]
    out["first_call"] = {
        "ttft_s": first["ttft_s"], "prompt_tokens": first["prefill_tokens"],
        "prefill_tok_s": (first["prefill_tokens"] / first["ttft_s"]
                          if first["ttft_s"] and first["prefill_tokens"] else None),
        "share_of_prefill_pct": 100.0 * (first["ttft_s"] or 0.0) / prefill if prefill else None,
    }

    if samples is not None and samples.rows:
        w = samples.window(wall0, wall1)
        pre = [(st["t_start"], st["t_start"] + (st["ttft_s"] or 0.0)) for st in steps]
        dec = [(st["t_start"] + (st["ttft_s"] or 0.0), st["t_end"]) for st in steps]
        out["gpu"] = {
            "mean_util_pct": w.mean_util,
            "duty_cycle_pct": None if w.idle_frac is None else 100.0 * (1.0 - w.idle_frac),
            "util_prefill_pct": _window_stats(samples, pre),
            "util_decode_pct": _window_stats(samples, dec),
            "energy_j": None if w.mean_power_w is None else w.mean_power_w * wall,
            "samples": w.n,
        }
    return out


def aggregate(decomps: list[dict], categories: dict[str, str]) -> dict:
    """Time-weighted bucket shares (sum of bucket / sum of wall) per task category."""
    groups: dict[str, list[dict]] = {}
    for d in decomps:
        if d.get("buckets_s"):
            groups.setdefault(categories.get(d["task"], "unknown"), []).append(d)
    res = {}
    for cat, ds in sorted(groups.items()):
        wall = sum(d["wall_s"] for d in ds)
        res[cat] = {"n": len(ds), "wall_s": wall,
                    "pct": {b: 100.0 * sum(d["buckets_s"][b] for d in ds) / wall for b in BUCKETS},
                    "not_ok": sum(not d["ok"] for d in ds)}
    return res


def to_markdown(agg: dict) -> str:
    lines = ["| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % | flagged |",
             "|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for cat, a in agg.items():
        p = a["pct"]
        lines.append(f"| {cat} | {a['n']} | {a['wall_s']:.0f} | {p['prefill']:.1f} | {p['decode']:.1f} "
                     f"| {p['tool']:.1f} | {p['harness']:.1f} | {p['unattributed']:.1f} | {a['not_ok']} |")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description="Time decomposition for every trial with a trace.json")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--gpu", type=Path)
    ap.add_argument("--tasks", type=Path, default=Path("tasks"))
    a = ap.parse_args()
    from harness.taskspec import load_all

    categories = {t.id: t.category for t in load_all(a.tasks)}
    samples = GpuSamples.from_csv(a.gpu or a.run / "gpu_samples.csv")
    decomps = []
    for p in sorted((a.run / "trials").glob("*/trace.json")):
        decomps.append(decompose(json.loads(p.read_text()), samples if samples.rows else None))
    (a.run / "decomposition.json").write_text(json.dumps(decomps, indent=2))
    agg = aggregate(decomps, categories)
    md = to_markdown(agg)
    (a.run / "decomposition.md").write_text(md + "\n")
    print(md)
    bad = [d["trial_id"] for d in decomps if not d["ok"]]
    print(f"\n{len(decomps)} trials, {len(bad)} flagged (unattributed >= {UNATTRIBUTED_LIMIT_PCT}% "
          f"or join warnings){': ' + ', '.join(bad) if bad else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
