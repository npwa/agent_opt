"""
analysis/determinism.py

Determinism of repeated trials (plan item P2, test of section 7.3): same task, same configuration, temperature 0,
run several times. If the stack were deterministic every repeat would produce the same text, the same tool
calls and the same number of tokens per call. Differences measure the noise floor that the confidence intervals
of every later experiment sit on top of (and how far n = 10 can be trusted).

* ``signature``     the agent's text parts and tool calls of one trial, in order, with the per-trial scratch
                    path replaced so that two trials of the same task are comparable
* ``analyse_run``   per task: how many repeats share the most common signature, where the others first diverge,
                    the share of calls whose completion-token count equals that of the reference repeat
                    (acceptance rule of 7.3: >= 90 % of calls), and whether the verdicts agree

    python -m analysis.determinism --run results/v1/determinism
"""

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

SCRATCH_RE = re.compile(r"/tmp/agent_opt_scratch/[^/\s\"\\]+/")


def signature(events: Path) -> list[tuple]:
    out = []
    for line in Path(events).read_text(errors="replace").splitlines():
        try:
            e = json.loads(line)
        except json.JSONDecodeError:
            continue
        part = e.get("part", {})
        if e.get("type") == "text":
            out.append(("text", SCRATCH_RE.sub("<WORK>/", part.get("text", ""))))
        elif e.get("type") == "tool_use":
            inp = json.dumps((part.get("state") or {}).get("input"), sort_keys=True)
            out.append(("tool", part.get("tool", ""), SCRATCH_RE.sub("<WORK>/", inp)))
    return out


def completion_tokens(metrics: Path) -> dict[str, list[int]]:
    """{trial_id: completion tokens of each LLM call, in order} from the proxy records."""
    calls = defaultdict(list)
    for line in Path(metrics).read_text().splitlines():
        try:
            r = json.loads(line)
        except json.JSONDecodeError:
            continue
        if r.get("trial_id") and r.get("status") == 200:
            calls[r["trial_id"]].append((r["wall_start"], r.get("completion_tokens")))
    return {t: [c for _, c in sorted(v)] for t, v in calls.items()}


def first_divergence(a: list, b: list) -> int | None:
    for i, (x, y) in enumerate(zip(a, b, strict=False)):
        if x != y:
            return i
    return None if len(a) == len(b) else min(len(a), len(b))


def analyse_run(run_dir: Path) -> dict:
    run_dir = Path(run_dir)
    tokens = completion_tokens(run_dir / "proxy" / "metrics.jsonl")
    by_task = defaultdict(list)
    for rec_path in sorted((run_dir / "trials").glob("*/record.json")):
        rec = json.loads(rec_path.read_text())
        ev = rec_path.parent / "events.jsonl"
        if rec.get("instrumented", True) and rec.get("status") != "harness_failed" and ev.exists():
            by_task[rec["task"]].append({"trial": rec["trial_id"], "sig": signature(ev), "passed": bool(rec.get("verdict_passed")),
                                         "tokens": tokens.get(rec["trial_id"], [])})
    tasks = {}
    for task, reps in sorted(by_task.items()):
        keys = [json.dumps(r["sig"]) for r in reps]
        modal_key, modal_n = Counter(keys).most_common(1)[0]
        ref = reps[keys.index(modal_key)]
        same_tok = total_tok = 0
        divs = []
        for r in reps:
            if r is ref:
                continue
            d = first_divergence(ref["sig"], r["sig"])
            divs.append(d)
            for a, b in zip(ref["tokens"], r["tokens"], strict=False):
                total_tok += 1
                same_tok += a == b
        tasks[task] = {
            "repeats": len(reps), "identical_to_modal": modal_n, "distinct_signatures": len(set(keys)),
            "first_divergence_steps": sorted(d for d in divs if d is not None),
            "calls_compared": total_tok, "calls_with_equal_tokens": same_tok,
            "call_token_agreement": same_tok / total_tok if total_tok else None,
            "pass_rate": sum(r["passed"] for r in reps) / len(reps),
            "verdicts_agree": len({r["passed"] for r in reps}) == 1,
        }
    tot, same = sum(t["calls_compared"] for t in tasks.values()), sum(t["calls_with_equal_tokens"] for t in tasks.values())
    return {"tasks": tasks, "call_token_agreement": same / tot if tot else None, "calls_compared": tot,
            "meets_90pct_rule": bool(tot and same / tot >= 0.9)}


def render(name: str, res: dict) -> str:
    L = [f"# Determinism: {name}", "",
         "Same task and configuration repeated at temperature 0. 'Identical' = same text parts and tool calls, in order.", "",
         "| Task | repeats | identical to the most common run | distinct runs | first divergence at step | calls with equal token count | verdicts agree |",
         "|---|---:|---:|---:|---|---:|---|"]
    for t, r in res["tasks"].items():
        ag = f"{r['calls_with_equal_tokens']}/{r['calls_compared']} ({r['call_token_agreement'] * 100:.0f} %)" if r["calls_compared"] else "n/a"
        L.append(f"| {t} | {r['repeats']} | {r['identical_to_modal']} | {r['distinct_signatures']} | "
                 f"{', '.join(map(str, r['first_divergence_steps'])) or '-'} | {ag} | {'yes' if r['verdicts_agree'] else 'no'} |")
    if res["calls_compared"]:
        L += ["", f"Over all tasks {res['call_token_agreement'] * 100:.0f} % of compared calls have the same completion-token count as "
              f"the reference repeat; the plan's rule (>= 90 %) is {'met' if res['meets_90pct_rule'] else 'NOT met'}."]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Determinism of repeated trials in a run directory")
    ap.add_argument("--run", type=Path, required=True)
    a = ap.parse_args()
    res = analyse_run(a.run)
    (a.run / "determinism.json").write_text(json.dumps(res, indent=2) + "\n")
    (a.run / "determinism.md").write_text(render(a.run.name, res))
    print(render(a.run.name, res))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
