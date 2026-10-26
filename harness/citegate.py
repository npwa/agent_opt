"""
harness/citegate.py

Post-hoc citation gate (plan item M6.4): the research answers of a run are checked with harness/citations.py, flagged citations are
handled in four ways and every result is checked again against the same evidence:

* ``none``     the answer as written
* ``mark``     flagged citations get ``[unverified]`` appended (no model call)
* ``drop``     flagged citations are cut out (no model call)
* ``rewrite``  one model call: remove or rephrase the flagged citations, keep everything else (needs the Ollama server)

Per arm: citations left, flagged share, supported citations kept (of those the original had), and words kept (usefulness proxy).

    python -m harness.citegate --run results/v1/m6_research --min-words 200 --out results/v1/m6_gate
"""

import argparse
import json
from collections.abc import Callable
from pathlib import Path

import httpx

from analysis import stats
from harness import citations as ci

SYSTEM = ("You edit a research answer. Some citations in it could not be found in the pages the author actually retrieved. "
          "Remove or rephrase every citation in the list so that none of its author names, years or URLs remains; if a statement depends on it, "
          "keep the finding in plain words and add '(source not verified)'. Leave everything else unchanged, including all other citations. "
          "Return only the edited answer.")


def rewrite_with_model(answer: str, flagged: list[str], base_url: str = "http://localhost:11434/v1", model: str = "qwen3.6-64k") -> str:
    msg = "Citations to remove or rephrase:\n" + "\n".join(f"- {x}" for x in flagged) + f"\n\nAnswer:\n{answer}"
    r = httpx.post(f"{base_url}/chat/completions", timeout=1800, json={
        "model": model, "temperature": 0, "seed": 1, "max_tokens": 12000, "stream": False,
        "reasoning_effort": "none",  # with thinking on, the first run spent its token budget on reasoning and returned empty answers
        "messages": [{"role": "system", "content": SYSTEM}, {"role": "user", "content": msg}]})
    return r.json()["choices"][0]["message"]["content"] or ""


def score(text: str, evidence: str, original_supported: set[str]) -> dict:
    res = ci.verify(ci.extract(text), evidence)
    kept = {c["raw"] for c in res if c["verdict"] == "supported"} & original_supported
    return {"citations": len(res), "flagged": sum(c["verdict"] != "supported" for c in res),
            "supported_kept": len(kept), "words": len(text.split())}


def evaluate(trials: list[dict], rewrite: Callable[[str, list[str]], str] | None) -> list[dict]:
    """trials: {"trial", "answer", "evidence"}; one row per trial and arm."""
    rows = []
    for t in trials:
        res = ci.verify(ci.extract(t["answer"]), t["evidence"])
        supported = {c["raw"] for c in res if c["verdict"] == "supported"}
        flagged = [c["raw"] for c in res if c["verdict"] != "supported"]
        arms = {"none": t["answer"], "mark": ci.gate(t["answer"], res, "mark"), "drop": ci.gate(t["answer"], res, "drop")}
        if rewrite is not None:
            arms["rewrite"] = rewrite(t["answer"], flagged) if flagged else t["answer"]
        for arm, text in arms.items():
            rows.append({"trial": t["trial"], "arm": arm, "original_supported": len(supported), "original_words": len(t["answer"].split()),
                         **score(text, t["evidence"], supported)})
    return rows


def render(rows: list[dict]) -> str:
    L = ["# Citation gate", "", "| Arm | trials | citations | flagged | flagged share (95% Wilson) | supported kept | words kept |", "|---|---:|---:|---:|---|---:|---:|"]
    for arm in dict.fromkeys(r["arm"] for r in rows):
        rs = [r for r in rows if r["arm"] == arm]
        n, f = sum(r["citations"] for r in rs), sum(r["flagged"] for r in rs)
        sup0, kept = sum(r["original_supported"] for r in rs), sum(r["supported_kept"] for r in rs)
        w0, w = sum(r["original_words"] for r in rs), sum(r["words"] for r in rs)
        lo, hi = stats.wilson_interval(f, n) if n else (0, 0)
        L.append(f"| {arm} | {len(rs)} | {n} | {f} | {f / n:.0%} [{lo:.0%}, {hi:.0%}] | {kept}/{sup0} | {w / w0:.0%} |" if n and w0 else f"| {arm} | {len(rs)} | {n} | {f} | | | |")
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Evaluate the post-hoc citation gate on a research run")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--min-words", type=int, default=200, help="only answers at least this long (finished answers)")
    ap.add_argument("--no-rewrite", action="store_true", help="skip the model call")
    ap.add_argument("--base-url", default="http://localhost:11434/v1")
    ap.add_argument("--model", default="qwen3.6-64k")
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    trials = []
    for d in sorted((a.run / "trials").iterdir()):
        if (d / "events.jsonl").exists():
            answer, evidence = ci.read_trial(d / "events.jsonl")
            if len(answer.split()) >= a.min_words:
                trials.append({"trial": d.name, "answer": answer, "evidence": evidence})
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "rewrites").mkdir(exist_ok=True)
    answers = {t["answer"]: t["trial"] for t in trials}

    def rewrite_and_keep(ans: str, flagged: list[str]) -> str:
        text = rewrite_with_model(ans, flagged, a.base_url, a.model)
        (a.out / "rewrites" / f"{answers[ans]}.md").write_text(text)
        return text
    rows = evaluate(trials, None if a.no_rewrite else rewrite_and_keep)
    (a.out / "gate.json").write_text(json.dumps(rows, indent=1) + "\n")
    (a.out / "gate.md").write_text(render(rows))
    print(render(rows))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
