"""
harness/promptbudget.py

Prompt-budget analysis (plan F13 / work item P8): where do the prompt tokens come from, and how much
prefill time do they cost?

Two independent parts, both CPU-only:

1. ``call_model`` -- from a run's proxy records (``metrics.jsonl``): the reported ``prompt_tokens``
   is the *full* prompt, so cache reuse is invisible there; it shows up in time-to-first-token. The
   model is  ttft = overhead + new_tokens / rate  where new_tokens is the growth of the prompt since
   the previous call of the same session (the previous prompt is cached) and ``rate`` is the cold
   prefill rate measured on first calls. It also checks whether first calls of *different* trials
   reuse a cached prefix (they do not if every first call runs at the cold rate).
2. ``component_budget`` -- from one captured chat-completion request body: characters (and an
   estimated token count) of the base system prompt, the environment block, the skills block, each
   tool schema and the user prompt. Tokens are apportioned by characters and calibrated to the real
   ``prompt_tokens`` of the same call, so they are estimates (about +-10 %).

    python -m harness.promptbudget --run results/v1/baseline [--request request.json]
"""

import argparse
import json
import re
import statistics as st
from collections import defaultdict
from pathlib import Path

# ---- 1. cache / prefill model from proxy records -------------------------------------------------------------------

def load_calls(metrics: Path) -> dict[tuple[str, str], list[dict]]:
    """Successful chat calls per (trial_id, attempt), in call order. Records without a trial id are ignored."""
    calls: dict[tuple[str, str], list[dict]] = defaultdict(list)
    for line in Path(metrics).read_text().splitlines():
        if not line.strip():
            continue
        r = json.loads(line)
        if r.get("trial_id") and r.get("status") == 200 and r.get("ttft_s") and r.get("prompt_tokens"):
            calls[(r["trial_id"], str(r.get("attempt", "1")))].append(r)
    for v in calls.values():
        v.sort(key=lambda r: r["wall_start"])
    return dict(calls)


def _median(xs: list[float]) -> float:
    return st.median(xs) if xs else float("nan")


def _ols(pairs: list[tuple[float, float]]) -> tuple[float, float] | None:
    """Least-squares y = a + b x; None when x has no spread."""
    n = len(pairs)
    if n < 3:
        return None
    mx, my = sum(x for x, _ in pairs) / n, sum(y for _, y in pairs) / n
    sxx = sum((x - mx) ** 2 for x, _ in pairs)
    if sxx <= 0:
        return None
    b = sum((x - mx) * (y - my) for x, y in pairs) / sxx
    return my - b * mx, b


def _followups(calls: dict[tuple[str, str], list[dict]]) -> list[tuple[int, float, int]]:
    """(new tokens since the previous call of the session, ttft, prompt tokens) for every follow-up call."""
    out = []
    for v in calls.values():
        for prev, cur in zip(v, v[1:], strict=False):
            new = cur["prompt_tokens"] - prev["prompt_tokens"]
            if new >= 0:
                out.append((new, cur["ttft_s"], cur["prompt_tokens"]))
    return out


def cold_rate(calls: dict[tuple[str, str], list[dict]]) -> float:
    """Cold prefill rate in tokens/s. Estimated from follow-up calls (ttft against new tokens, whose prefix is
    cached), because first calls can be warm; falls back to the median first-call rate if that fit is unusable."""
    fit = _ols([(n, t) for n, t, _ in _followups(calls)])
    if fit and fit[1] > 0:
        return 1.0 / fit[1]
    firsts = [v[0] for v in calls.values() if v]
    return _median([c["prompt_tokens"] / c["ttft_s"] for c in firsts])


def call_model(calls: dict[tuple[str, str], list[dict]]) -> dict:
    """Fit ttft = overhead + new_tokens / rate and report how much of the prompt is served from cache."""
    firsts = [v[0] for v in calls.values() if v]
    rate = cold_rate(calls)
    pairs = _followups(calls)
    overhead = _median([t - n / rate for n, t, _ in pairs]) if pairs else 0.0
    errs = [abs(t - (overhead + n / rate)) for n, t, _ in pairs]
    cached_share = [1 - min(p, max(0.0, (t - overhead) * rate)) / p for _, t, p in pairs]
    first_cold = [c for c in firsts if c["ttft_s"] >= 0.85 * c["prompt_tokens"] / rate]  # within 15 % of a full cold prefill
    total_first = sum(c["ttft_s"] for c in firsts)
    total_follow = sum(t for _, t, _ in pairs)
    return {
        "trials": len(calls),
        "calls": sum(len(v) for v in calls.values()),
        "cold_rate_tok_s": rate,
        "first_call_prompt_tokens_median": _median([c["prompt_tokens"] for c in firsts]),
        "first_call_ttft_s": {"median": _median([c["ttft_s"] for c in firsts]),
                              "min": min((c["ttft_s"] for c in firsts), default=float("nan")),
                              "max": max((c["ttft_s"] for c in firsts), default=float("nan"))},
        "first_calls_cold": len(first_cold),
        "first_calls_warm": len(firsts) - len(first_cold),
        "followup_overhead_s": overhead,
        "followup_model_median_abs_err_s": _median(errs),
        "followup_cached_share_of_prompt_median": _median(cached_share),
        "first_call_ttft_total_s": total_first,
        "followup_ttft_total_s": total_follow,
        "first_call_share_of_prefill": total_first / (total_first + total_follow) if total_first + total_follow else 0.0,
    }


def cross_trial_reuse(calls: dict[tuple[str, str], list[dict]], rate: float | None = None) -> dict:
    """Do first calls reuse a prefix cached by an earlier trial? Compare their ttft with a full cold prefill."""
    rate = rate or cold_rate(calls)
    firsts = sorted((v[0] for v in calls.values() if v), key=lambda c: c["wall_start"])
    speedup = [(c["prompt_tokens"] / rate) / c["ttft_s"] for c in firsts]  # 1.0 = fully cold, > 1 = prefix reused
    prev_task = [None, *[c.get("task") for c in firsts[:-1]]]
    same = [s for s, c, p in zip(speedup, firsts, prev_task, strict=True) if p is not None and p == c.get("task")]
    other = [s for s, c, p in zip(speedup, firsts, prev_task, strict=True) if p is not None and p != c.get("task")]
    return {
        "first_calls": len(firsts),
        "speedup_vs_cold_median_same_task_as_previous": _median(same) if same else None,
        "n_same_task_as_previous": len(same),
        "speedup_vs_cold_median_other_task": _median(other) if other else None,
        "n_other_task": len(other),
        "fraction_of_first_calls_cold": sum(s < 1.15 for s in speedup) / len(speedup) if speedup else None,
    }


def elapsed_by_trial(run_dir: Path) -> dict[str, float]:
    out = {}
    for f in Path(run_dir, "trials").glob("*/record.json"):
        r = json.loads(f.read_text())
        if r.get("elapsed_s") is not None:
            out[r["trial_id"]] = r["elapsed_s"]
    return out


# ---- 2. component budget from one captured request ---------------------------------------------------------------

def _text(content) -> str:
    if isinstance(content, str):
        return content
    return "".join(p.get("text", "") for p in content if isinstance(p, dict))


def split_system(system: str) -> dict[str, int]:
    """Characters of the base prompt, environment block and skills block of an OpenCode system message."""
    env = system.find("<env>")
    skills = system.find("Skills provide specialized instructions")
    if skills < 0:
        skills = system.find("<available_skills>")
    cuts = sorted(i for i in (env, skills) if i >= 0)
    if not cuts:
        return {"base_prompt": len(system)}
    out = {"base_prompt": cuts[0]}
    if env >= 0:
        end = skills if skills > env else len(system)
        out["environment"] = end - env
    if skills >= 0:
        out["skills_block"] = len(system) - skills
    return out


def component_budget(body: dict, prompt_tokens: int) -> dict:
    """Chars and estimated tokens per prompt component of one chat-completion request body."""
    msgs = body.get("messages", [])
    system = "".join(_text(m["content"]) for m in msgs if m.get("role") == "system")
    rest = "".join(_text(m.get("content") or "") for m in msgs if m.get("role") != "system")
    tools = {t["function"]["name"]: len(json.dumps(t["function"], separators=(",", ":")))
             for t in body.get("tools", [])}
    comps = {f"system:{k}": v for k, v in split_system(system).items()}
    comps["user_and_history"] = len(rest)
    comps.update({f"tool:{k}": v for k, v in tools.items()})
    total = sum(comps.values())
    per_char = prompt_tokens / total if total else 0.0
    rows = [{"component": k, "chars": v, "tokens_est": round(v * per_char)} for k, v in comps.items()]
    skills_n = len(re.findall(r"<skill>", system))
    groups = {"system prompt (base)": comps.get("system:base_prompt", 0),
              "environment block": comps.get("system:environment", 0),
              "skills block": comps.get("system:skills_block", 0),
              "tool schemas": sum(tools.values()), "user prompt": len(rest)}
    return {"prompt_tokens": prompt_tokens, "chars_total": total, "chars_per_token": 1 / per_char if per_char else None,
            "skills_listed": skills_n, "tools": len(tools),
            "groups": [{"component": k, "chars": v, "tokens_est": round(v * per_char),
                        "share": v / total if total else 0.0} for k, v in groups.items()],
            "components": rows}


# ---- report -------------------------------------------------------------------------------------------------------------

def render(run: str, model: dict, reuse: dict, elapsed_total: float | None, comp: dict | None) -> str:
    L = [f"# Prompt budget: {run}", "",
         f"{model['trials']} trials, {model['calls']} LLM calls (proxy records).", "",
         "## Prefill and cache", "",
         f"- Cold prefill rate: **{model['cold_rate_tok_s']:.0f} tok/s** (fitted on follow-up calls). First call = "
         f"{model['first_call_prompt_tokens_median']:.0f} prompt tokens; ttft median {model['first_call_ttft_s']['median']:.1f} s "
         f"(min {model['first_call_ttft_s']['min']:.1f}, max {model['first_call_ttft_s']['max']:.1f}); "
         f"{model['first_calls_cold']} cold, {model['first_calls_warm']} warm.",
         f"- Follow-up calls: ttft = {model['followup_overhead_s']:.2f} s + new tokens / rate "
         f"(median abs. error {model['followup_model_median_abs_err_s']:.2f} s); "
         f"{model['followup_cached_share_of_prompt_median'] * 100:.0f} % of the prompt is served from cache (median).",
         f"- First calls are {model['first_call_share_of_prefill'] * 100:.0f} % of all prefill time "
         f"({model['first_call_ttft_total_s']:.0f} s of {model['first_call_ttft_total_s'] + model['followup_ttft_total_s']:.0f} s)."
         + (f" That is {model['first_call_ttft_total_s'] / elapsed_total * 100:.0f} % of the summed trial time." if elapsed_total else ""),
         f"- Cross-trial reuse: {reuse['fraction_of_first_calls_cold'] * 100:.0f} % of first calls are fully cold. Speed-up of a first "
         f"call over a full cold prefill (median; 1.0 = no reuse): "
         + (f"{reuse['speedup_vs_cold_median_other_task']:.1f} after a different task" if reuse["speedup_vs_cold_median_other_task"] is not None else "n/a after a different task")
         + (f", {reuse['speedup_vs_cold_median_same_task_as_previous']:.1f} after the same task (n = {reuse['n_same_task_as_previous']})."
            if reuse["speedup_vs_cold_median_same_task_as_previous"] is not None else ".")]
    if comp:
        L += ["", "## Prompt components (one captured first call)", "",
              f"{comp['prompt_tokens']} prompt tokens, {comp['chars_total']} characters "
              f"(about {comp['chars_per_token']:.1f} chars/token); {comp['tools']} tools; {comp['skills_listed']} skills listed. "
              "Token counts are estimates (characters apportioned to the real prompt size).", "",
              "| Component | chars | tokens (est.) | share |", "|---|---:|---:|---:|"]
        L += [f"| {g['component']} | {g['chars']} | {g['tokens_est']} | {g['share'] * 100:.0f} % |" for g in comp["groups"]]
        L += ["", "| Tool | chars | tokens (est.) |", "|---|---:|---:|"]
        L += [f"| {c['component'][5:]} | {c['chars']} | {c['tokens_est']} |" for c in comp["components"]
              if c["component"].startswith("tool:")]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Prompt-budget analysis of a run (cache model + component budget)")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--metrics", type=Path, help="default: <run>/proxy/metrics.jsonl")
    ap.add_argument("--request", type=Path, help="one captured chat-completion request body (JSON)")
    ap.add_argument("--request-prompt-tokens", type=int, help="real prompt_tokens of that call (default: median first call)")
    ap.add_argument("--out", type=Path, help="default: <run>")
    a = ap.parse_args()
    calls = load_calls(a.metrics or a.run / "proxy" / "metrics.jsonl")
    model, reuse = call_model(calls), cross_trial_reuse(calls)
    elapsed = elapsed_by_trial(a.run)
    total = sum(elapsed.get(k[0], 0.0) for k in calls) or None
    comp = None
    if a.request:
        pt = a.request_prompt_tokens or round(model["first_call_prompt_tokens_median"])
        comp = component_budget(json.loads(a.request.read_text()), pt)
    out = a.out or a.run
    out.mkdir(parents=True, exist_ok=True)
    (out / "promptbudget.md").write_text(render(a.run.name, model, reuse, total, comp))
    (out / "promptbudget.json").write_text(json.dumps({"model": model, "cross_trial_reuse": reuse, "components": comp},
                                                      indent=2, sort_keys=True) + "\n")
    print(f"prompt budget for {a.run.name}: {model['calls']} calls -> {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
