"""
analysis/nsys_summary.py

What share of an agent's wall-clock is the GPU actually working? (success criterion 4, plan M4.3)

Input: a directory of Nsight Systems captures made with ``python -m harness.nsys capture`` (one trial each; every
capture directory holds ``nsys_alignment.json``, the aligned trace in ``run/trials/*/trace.json`` and the trial record).
Output: one table row per capture and per task category (mean over its captures):

* wall-clock of the trial and its split into prefill (time to first token), decode and the rest, from the trace
* **GPU kernel-active time** (kernels plus CUDA graphs) in seconds and as a share of the wall-clock, of the prefill
  windows and of the decode windows
* NVML's utilisation for the same LLM windows: "any kernel ran in the sample window", the coarse signal the earlier
  reports used (the Nsight pilot found 10 % real activity against 50 % NVML)

    python -m analysis.nsys_summary --captures results/v1/m4_3 --out results/v1/m4_3
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path

import yaml


def categories(tasks_dir: Path) -> dict[str, str]:
    return {s["id"]: s.get("category", "") for s in (yaml.safe_load(f.read_text()) for f in Path(tasks_dir).glob("*.yaml"))}


def capture_row(cap_dir: Path, cats: dict[str, str]) -> dict | None:
    """One row for a capture directory, or None when it is incomplete (no alignment or no trace)."""
    al_path = cap_dir / "nsys_alignment.json"
    traces = sorted((cap_dir / "run" / "trials").glob("*/trace.json"))
    if not al_path.exists() or not traces:
        return None
    al, tr = json.loads(al_path.read_text()), json.loads(traces[0].read_text())
    rec_path = traces[0].with_name("record.json")
    rec = json.loads(rec_path.read_text()) if rec_path.exists() else {}
    s, steps = tr["summary"], {st["step"]: st for st in tr["steps"]}
    pre_active = dec_active = 0.0
    for a in al["steps"]:
        st = steps.get(a["step"])
        if st:
            pre_active += (a["kernel_busy_pct_prefill"] or 0) / 100 * st["ttft_s"]
            dec_active += (a["kernel_busy_pct_decode"] or 0) / 100 * st["decode_s"]
    gaps = [a["launch_gaps_decode"] for a in al["steps"] if a["launch_gaps_decode"]["n"]]
    wall = s["wall_s"]
    active = al["summary"]["kernel_active_s_in_run"]
    return {
        "capture": cap_dir.name, "task": tr["task"], "category": cats.get(tr["task"], "?"), "status": rec.get("status", tr.get("status")),
        "wall_s": wall, "prefill_s": s["ttft_s"], "decode_s": s["decode_s"], "other_s": wall - s["ttft_s"] - s["decode_s"],
        "steps": s["n_steps"], "kernel_active_s": active, "gpu_active_pct_of_wall": active / wall * 100,
        "gpu_active_pct_of_prefill": pre_active / s["ttft_s"] * 100 if s["ttft_s"] else None,
        "gpu_active_pct_of_decode": dec_active / s["decode_s"] * 100 if s["decode_s"] else None,
        "nvml_busy_pct_of_llm": al["summary"]["nvml_busy_pct_llm"],
        "decode_gap_p50_ms": sorted(g["p50_ms"] for g in gaps)[len(gaps) // 2] if gaps else None,
        "decode_gap_p99_ms": max((g["p99_ms"] for g in gaps), default=None),
        "kernels_in_llm_windows_pct": al["summary"]["kernel_time_in_llm_windows_pct"],
    }


def _mean(xs: list) -> float | None:
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else None


def by_category(rows: list[dict]) -> list[dict]:
    groups = defaultdict(list)
    for r in rows:
        groups[r["category"]].append(r)
    keys = ("wall_s", "prefill_s", "decode_s", "other_s", "kernel_active_s", "gpu_active_pct_of_wall", "gpu_active_pct_of_prefill",
            "gpu_active_pct_of_decode", "nvml_busy_pct_of_llm")
    return [{"category": c, "captures": len(rs), **{k: _mean([r[k] for r in rs]) for k in keys}} for c, rs in sorted(groups.items())]


def _f(v, d=1) -> str:
    return "n/a" if v is None else f"{v:.{d}f}"


def render(rows: list[dict], cats: list[dict], skipped: list[str]) -> str:
    tot = {k: sum(r[k] for r in rows) for k in ("wall_s", "prefill_s", "decode_s", "kernel_active_s")}
    L = ["# Share of wall-clock the GPU is actually working", "",
         f"{len(rows)} Nsight Systems captures (one trial each), {len(cats)} task categories. 'GPU active' = time with at least one "
         "kernel or CUDA graph running on the GPU, from the Nsight trace; NVML is the coarse 'any kernel in the sample window' "
         "signal used by the earlier reports.", "",
         f"**All captures together: the GPU is active {tot['kernel_active_s'] / tot['wall_s'] * 100:.1f} % of the wall-clock** "
         f"({tot['kernel_active_s']:.0f} s of {tot['wall_s']:.0f} s); prefill is {tot['prefill_s'] / tot['wall_s'] * 100:.0f} % and decode "
         f"{tot['decode_s'] / tot['wall_s'] * 100:.0f} % of the wall-clock.", "",
         "## Per task category (mean of its captures)", "",
         "| Category | captures | wall s | prefill s | decode s | GPU active s | GPU active % of wall | % of prefill | % of decode | NVML busy % of LLM time |",
         "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for c in cats:
        L.append(f"| {c['category']} | {c['captures']} | {_f(c['wall_s'], 0)} | {_f(c['prefill_s'], 0)} | {_f(c['decode_s'], 0)} | "
                 f"{_f(c['kernel_active_s'])} | {_f(c['gpu_active_pct_of_wall'])} | {_f(c['gpu_active_pct_of_prefill'])} | "
                 f"{_f(c['gpu_active_pct_of_decode'])} | {_f(c['nvml_busy_pct_of_llm'], 0)} |")
    L += ["", "## Per capture", "",
          "| Capture | status | wall s | steps | GPU active % of wall | % of prefill | % of decode | NVML % | decode gap p50 / p99 ms |",
          "|---|---|---:|---:|---:|---:|---:|---:|---|"]
    for r in rows:
        L.append(f"| {r['capture']} | {r['status']} | {_f(r['wall_s'], 0)} | {r['steps']} | {_f(r['gpu_active_pct_of_wall'])} | "
                 f"{_f(r['gpu_active_pct_of_prefill'])} | {_f(r['gpu_active_pct_of_decode'])} | {_f(r['nvml_busy_pct_of_llm'], 0)} | "
                 f"{_f(r['decode_gap_p50_ms'], 2)} / {_f(r['decode_gap_p99_ms'], 1)} |")
    if skipped:
        L += ["", "Captures without a usable result (skipped): " + ", ".join(skipped) + "."]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Aggregate Nsight captures per task category")
    ap.add_argument("--captures", type=Path, required=True, help="directory holding one sub-directory per capture")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--tasks", type=Path, default=Path("tasks"))
    a = ap.parse_args()
    cats_map = categories(a.tasks)
    rows, skipped = [], []
    for d in sorted(p for p in a.captures.iterdir() if p.is_dir()):
        r = capture_row(d, cats_map)
        (rows if r else skipped).append(r or d.name)
    if not rows:
        print("no usable capture found")
        return 1
    cats = by_category(rows)
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "RESULTS.md").write_text(render(rows, cats, skipped))
    (a.out / "summary.json").write_text(json.dumps({"captures": rows, "categories": cats, "skipped": skipped}, indent=2) + "\n")
    print(render(rows, cats, skipped))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
