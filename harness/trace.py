"""
harness/trace.py

Per-trial trace (F5): joins three sources onto one epoch-time axis.

* OpenCode event stream (``events.jsonl``): step boundaries, tool calls with start/end times
* proxy records (``metrics.jsonl``): one per LLM call - start, TTFT, token counts, chunk times
* GPU samples (``gpu_samples.csv``): utilisation/power at ~10 Hz

Definitions
-----------
A *step* is one LLM call plus the tool calls its output triggered. The i-th OpenCode step is
paired with the i-th proxy record of the same trial attempt (sorted by start time); a count
mismatch, or an OpenCode ``step_start`` (= first token) that disagrees with the proxy's
t_start + ttft by more than ``ALIGN_TOL_S``, is reported in ``warnings`` instead of being hidden.

* LLM window  = [t_start, t_end) of the proxy record (windows are half-open for GPU samples)
* gap window  = [t_end, next step's t_start] (last step: trial end) - tool execution plus harness
  overhead, the time the GPU has nothing to do for this agent
* ``idle_gap_s`` = gap seconds x fraction of GPU samples in the gap at or below IDLE_UTIL_PCT
* ITL = differences between consecutive proxy chunk arrival times (chunks may batch several
  SSE events, so this approximates inter-token latency)

All times are epoch seconds (``time.time()`` domain); every source is on the same machine.
"""

import argparse
import json
import sys
from pathlib import Path

from harness.telemetry import GpuSamples

ALIGN_TOL_S = 2.0


def percentile(values: list[float], q: float) -> float | None:
    """Linear-interpolation percentile (q in 0..100), same as numpy's default."""
    if not values:
        return None
    s = sorted(values)
    pos = (len(s) - 1) * q / 100.0
    lo = int(pos)
    hi = min(lo + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (pos - lo)


def load_event_steps(path: Path) -> list[dict]:
    """OpenCode `--format json` events -> list of steps with tool calls (times in epoch s)."""
    steps: list[dict] = []
    cur: dict | None = None
    if not Path(path).exists():
        return steps
    for line in Path(path).read_text(errors="replace").splitlines():
        try:
            e = json.loads(line)
        except json.JSONDecodeError:
            continue
        ts = e.get("timestamp", 0) / 1000.0
        part = e.get("part", {})
        if e["type"] == "step_start":
            cur = {"start": ts, "finish": None, "tools": [], "input": 0, "output": 0, "reason": None}
            steps.append(cur)
        elif cur is None:
            continue
        elif e["type"] == "tool_use":
            t = part.get("state", {}).get("time", {})
            start = t.get("start", e.get("timestamp", 0)) / 1000.0
            end = t.get("end", e.get("timestamp", 0)) / 1000.0
            cur["tools"].append({"tool": part.get("tool", "?"), "start": start, "end": end,
                                 "status": part.get("state", {}).get("status")})
        elif e["type"] == "step_finish":
            tok = part.get("tokens", {})
            cur.update(finish=ts, input=tok.get("input", 0), output=tok.get("output", 0),
                       reason=part.get("reason"))
    return steps


def load_proxy_rows(metrics: Path, trial_id: str, attempt: int) -> list[dict]:
    rows = []
    if Path(metrics).exists():
        for line in Path(metrics).read_text().splitlines():
            try:
                r = json.loads(line)
            except json.JSONDecodeError:
                continue
            if r.get("trial_id") == trial_id and str(r.get("attempt")) == str(attempt):
                rows.append(r)
    return sorted(rows, key=lambda r: r["wall_start"])


def _gpu(samples: GpuSamples | None, t0: float, t1: float) -> dict:
    if samples is None or t1 <= t0:
        return {"busy_pct": None, "idle_frac": None, "power_w": None, "n": 0}
    w = samples.window(t0, t1)
    return {"busy_pct": w.mean_util, "idle_frac": w.idle_frac, "power_w": w.mean_power_w, "n": w.n}


def build_trace(record: dict, ev_steps: list[dict], proxy_rows: list[dict],
                samples: GpuSamples | None) -> dict:
    tid = record["trial_id"]
    warnings: list[str] = []
    n = min(len(ev_steps), len(proxy_rows))
    if len(ev_steps) != len(proxy_rows):
        warnings.append(f"count_mismatch: {len(ev_steps)} OpenCode steps vs {len(proxy_rows)} "
                        "proxy records")
    wall_end = record.get("wall_end")
    steps = []
    for i in range(n):
        ev, px = ev_steps[i], proxy_rows[i]
        t_start = px["wall_start"]
        t_end = t_start + px["total_s"]
        next_start = proxy_rows[i + 1]["wall_start"] if i + 1 < n else (wall_end or t_end)
        next_start = max(next_start, t_end)
        ttft = px.get("ttft_s")
        decode_s = (px["total_s"] - ttft) if ttft is not None else None
        ct = px.get("chunk_t", [])
        deltas = [(b - a) * 1000 for a, b in zip(ct, ct[1:], strict=False)]
        tool_lat = sum(max(0.0, t["end"] - t["start"]) for t in ev["tools"])
        # OpenCode stamps step_start when the first token arrives (verified on real runs: the
        # offset from the proxy's request start equals the proxy-measured TTFT), so the
        # consistency check is against t_start + ttft, which also cross-validates the TTFT.
        first_token_skew = ev["start"] - (t_start + (ttft or 0.0))
        if abs(first_token_skew) > ALIGN_TOL_S:
            warnings.append(f"step {i + 1}: OpenCode step_start differs from proxy first token "
                            f"by {first_token_skew:+.1f}s")
        llm = _gpu(samples, t_start, t_end)
        gap = _gpu(samples, t_end, next_start)
        gap_s = next_start - t_end
        steps.append({
            "step": i + 1, "llm_call_id": f"{tid}#a{px.get('attempt')}:{i + 1}",
            "t_start": t_start, "t_end": t_end, "ttft_s": ttft, "llm_s": px["total_s"],
            "decode_s": decode_s,
            "prefill_tokens": px.get("prompt_tokens"), "decode_tokens": px.get("completion_tokens"),
            "prefill_tok_s": px.get("input_tok_s"), "decode_tok_s": px.get("output_tok_s"),
            "n_chunks": px.get("chunks"), "first_token_skew_s": first_token_skew,
            "itl_p50_ms": percentile(deltas, 50), "itl_p99_ms": percentile(deltas, 99),
            "tool_names": [t["tool"] for t in ev["tools"]], "tool_latency_s": tool_lat,
            "tools": ev["tools"], "oc_step_start": ev["start"], "oc_step_finish": ev["finish"],
            "tool_errors": sum(t["status"] == "error" for t in ev["tools"]),
            "gap_s": gap_s,
            "gpu_busy_pct_llm": llm["busy_pct"], "gpu_power_w_llm": llm["power_w"],
            "gpu_samples_llm": llm["n"],
            "gpu_busy_pct_gap": gap["busy_pct"], "gpu_samples_gap": gap["n"],
            "idle_gap_s": gap_s * gap["idle_frac"] if gap["idle_frac"] is not None else None,
        })
    unmatched = [{"start": e["start"], "finish": e["finish"]} for e in ev_steps[n:]
                 if e["finish"] is not None]
    summary = {
        "wall_start": record.get("wall_start"), "wall_end": wall_end,
        "wall_s": (wall_end - record["wall_start"]) if wall_end and record.get("wall_start") else None,
        "n_steps": len(steps),
        "llm_s": sum(s["llm_s"] for s in steps),
        "ttft_s": sum(s["ttft_s"] or 0.0 for s in steps),
        "decode_s": sum(s["decode_s"] or 0.0 for s in steps),
        "tool_s": sum(s["tool_latency_s"] for s in steps),
        "gap_s": sum(s["gap_s"] for s in steps),
        "idle_gap_s": sum(s["idle_gap_s"] or 0.0 for s in steps),
        "prefill_tokens": sum(s["prefill_tokens"] or 0 for s in steps),
        "decode_tokens": sum(s["decode_tokens"] or 0 for s in steps),
        "gpu_telemetry": samples is not None and bool(samples.rows),
    }
    return {"trial_id": tid, "task": record.get("task"), "config": record.get("config"),
            "attempt": record.get("attempt"), "status": record.get("status"),
            "summary": summary, "steps": steps, "unmatched_event_steps": unmatched,
            "warnings": warnings}


def build_trial_trace(trial_dir: Path, metrics: Path, samples: GpuSamples | None) -> dict | None:
    rec_path = Path(trial_dir) / "record.json"
    if not rec_path.exists():
        return None
    record = json.loads(rec_path.read_text())
    if record.get("status") == "harness_failed" or record.get("instrumented") is False:
        return None  # no proxy records exist for harness failures / uninstrumented parity trials
    if not (Path(trial_dir) / "events.jsonl").exists():
        return None  # recorded without the event stream (e.g. a records-only archive)
    attempt = record.get("attempt", 1)
    return build_trace(record, load_event_steps(Path(trial_dir) / "events.jsonl"),
                       load_proxy_rows(metrics, record["trial_id"], attempt), samples)


def build_all(run_dir: Path, metrics: Path | None = None, gpu_csv: Path | None = None) -> list[dict]:
    """Write trace.json next to every finished trial's record.json."""
    run_dir = Path(run_dir)
    metrics = metrics or run_dir / "proxy" / "metrics.jsonl"
    samples = GpuSamples.from_csv(gpu_csv or run_dir / "gpu_samples.csv")
    out = []
    for d in sorted((run_dir / "trials").glob("*")):
        tr = build_trial_trace(d, metrics, samples if samples.rows else None)
        if tr:
            (d / "trace.json").write_text(json.dumps(tr, indent=2))
            out.append(tr)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Build trace.json for every finished trial of a run")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--metrics", type=Path)
    ap.add_argument("--gpu", type=Path)
    a = ap.parse_args()
    traces = build_all(a.run, a.metrics, a.gpu)
    for t in traces:
        s = t["summary"]
        print(f"{t['trial_id']}: steps={s['n_steps']} wall={s['wall_s'] and round(s['wall_s'], 1)}s "
              f"llm={s['llm_s']:.1f}s tool={s['tool_s']:.1f}s gap={s['gap_s']:.1f}s "
              f"idle_gap={s['idle_gap_s']:.1f}s warnings={len(t['warnings'])}")
    print(f"{len(traces)} traces written")
    return 0


if __name__ == "__main__":
    sys.exit(main())
