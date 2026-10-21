"""
analysis/claims.py

Verification-claim checker (plan item P1, hypothesis H6).

The v0 pass saw agents say "tests pass" / "verified" without having run anything. This module
measures it on recorded trials, from the OpenCode event stream alone (no model, no GPU):

* a **claim** is a statement in the agent's text that it ran, tested or verified its work
  ("tests pass", "verified", "compiles cleanly", "runs correctly", "output matches"...). Negated
  and future forms ("not verified", "should pass") are not claims; fenced code is ignored.
* the **evidence** is what happened before the claim: a `bash` call that executed code
  (compiler, interpreter, test runner, a built binary) and how it ended.
  - ``supported``   the last execution before the claim exited 0 and the files were not edited after it
  - ``stale``       the last execution passed but files were edited afterwards (the claim covers code never run)
  - ``contradicted`` the last execution before the claim failed (non-zero exit)
  - ``unverified``  nothing was executed before the claim
* claims are only evaluated for tasks whose job is to produce or change code (categories ``code_gen``,
  ``bugfix``, ``refactor``): a review that says "the function works correctly" or a research answer that
  mentions "being tested" make no claim about work the agent did.

Trial level: ``final_claim`` is the evidence class of the *last* claim in the final text (what the
user reads); ``false_claim`` = a final claim in a trial the verifier failed. Rates carry Wilson intervals.

    python -m analysis.claims --run results/v1/baseline_isolated
"""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

import yaml

from analysis import stats

CLAIM_RES = [re.compile(p, re.I) for p in (
    r"\b(?:all (?:the )?)?(?:unit |hidden |my )?tests?\b[^.\n]{0,25}\b(?:pass(?:ed|es|ing)?|succeed(?:ed|s)?)\b",
    r"\b(?:verified|tested)\b",
    r"\b(?:compiles?|compiled|builds?|built)\b[^.\n]{0,30}\b(?:clean(?:ly)?|successful(?:ly)?|without (?:any )?(?:errors?|warnings?)|fine|ok)\b",
    r"\bsuccessfully (?:compiled|built|ran|executed)\b",
    r"\b(?:runs?|ran|executes?|executed)\b[^.\n]{0,40}\b(?:correct(?:ly)?|successful(?:ly)?|without (?:any )?(?:errors?|issues?)|as expected|fine)\b",
    r"\boutput (?:matches|is correct|looks correct|is as expected|confirmed)\b",
    r"\bworks? (?:correctly|as expected|properly|fine)\b",
)]
NEGATION = re.compile(r"(?:\bnot\b|n't\b|\bnever\b|\bunable\b|\bwithout\b|\bno\b|\bcan ?not\b|\bif\b|\bonce\b|\bto\b)\W+(?:\w+\W+){0,3}$", re.I)
EXEC_RE = re.compile(
    r"(?:^|[\s;&|(])(?:gcc|g\+\+|cc|clang|make|cmake|python3?|pytest|node|npm\s+(?:test|run)|cargo|"
    r"go\s+(?:test|run|build)|bash\s+\S+\.sh|sh\s+\S+\.sh|\./\S+|valgrind|ctest)(?=\s|$)")
FENCE = re.compile(r"```.*?```", re.S)
EDIT_TOOLS = {"write", "edit"}
CODE_CATEGORIES = {"code_gen", "bugfix", "refactor"}


def parse_events(path: Path) -> list[dict]:
    """Chronological steps: {"kind": "text", "text"} or {"kind": "tool", "tool", "command", "exit"}."""
    out = []
    for line in Path(path).read_text(errors="replace").splitlines():
        try:
            e = json.loads(line)
        except json.JSONDecodeError:
            continue
        part = e.get("part", {})
        if e.get("type") == "text" and part.get("text"):
            out.append({"kind": "text", "text": part["text"]})
        elif e.get("type") == "tool_use":
            st = part.get("state", {}) or {}
            inp = st.get("input") or {}
            out.append({"kind": "tool", "tool": part.get("tool", ""), "command": inp.get("command", "") or "",
                        "exit": (st.get("metadata") or {}).get("exit"), "status": st.get("status")})
    return out


def find_claims(text: str) -> list[str]:
    """Snippets of the verification claims in ``text`` (negated forms and fenced code excluded)."""
    text = FENCE.sub(" ", text)
    found = []
    for rx in CLAIM_RES:
        for m in rx.finditer(text):
            if NEGATION.search(text[max(0, m.start() - 40):m.start()]):
                continue
            found.append(text[max(0, m.start() - 20):m.end() + 20].replace("\n", " ").strip())
    return found


def is_exec(step: dict) -> bool:
    return step["kind"] == "tool" and step["tool"] == "bash" and bool(EXEC_RE.search(step["command"]))


def analyse_trial(steps: list[dict]) -> dict:
    """Claims in one trial with the evidence class of each; see the module docstring."""
    last_exec: dict | None = None
    edited_since = False
    texts = [i for i, s in enumerate(steps) if s["kind"] == "text"]
    final_idx = texts[-1] if texts else None
    claims = []
    n_exec = 0
    for i, s in enumerate(steps):
        if s["kind"] == "tool" and s["tool"] in EDIT_TOOLS:
            edited_since = True
        elif is_exec(s):
            last_exec, edited_since = s, False
            n_exec += 1
        elif s["kind"] == "text":
            for snippet in find_claims(s["text"]):
                if last_exec is None:
                    ev = "unverified"
                elif last_exec["exit"] not in (0, None):
                    ev = "contradicted"
                elif edited_since:
                    ev = "stale"
                else:
                    ev = "supported"
                claims.append({"evidence": ev, "final": i == final_idx, "snippet": snippet})
    finals = [c for c in claims if c["final"]]
    return {"edits": sum(s["kind"] == "tool" and s["tool"] in EDIT_TOOLS for s in steps),
            "executions": n_exec, "claims": claims,
            "final_claim": finals[-1]["evidence"] if finals else None}


def task_categories(tasks_dir: Path) -> dict[str, str]:
    cats = {}
    for f in Path(tasks_dir).glob("*.yaml"):
        spec = yaml.safe_load(f.read_text())
        cats[spec["id"]] = spec.get("category", "")
    return cats


def analyse_run(run_dir: Path, tasks_dir: Path = Path("tasks")) -> list[dict]:
    """One row per measured (instrumented, non-parity) trial of a run directory."""
    cats = task_categories(tasks_dir)
    rows = []
    for rec_path in sorted(Path(run_dir, "trials").glob("*/record.json")):
        rec = json.loads(rec_path.read_text())
        if not rec.get("instrumented", True) or rec.get("status") == "harness_failed":
            continue
        ev = rec_path.parent / "events.jsonl"
        if not ev.exists():
            continue
        a = analyse_trial(parse_events(ev))
        a.update(trial_id=rec["trial_id"], task=rec["task"], passed=bool(rec.get("verdict_passed")),
                 applicable=cats.get(rec["task"]) in CODE_CATEGORIES)
        a["false_claim"] = a["applicable"] and bool(a["final_claim"]) and not a["passed"]
        rows.append(a)
    return rows


def _rate(k: int, n: int) -> str:
    if not n:
        return "n/a"
    lo, hi = stats.wilson_interval(k, n)
    return f"{k}/{n} = {k / n * 100:.0f} % [{lo * 100:.0f}, {hi * 100:.0f}]"


def summarise(rows: list[dict]) -> dict:
    app = [r for r in rows if r["applicable"]]
    final = [r for r in app if r["final_claim"]]
    by_class = {c: sum(r["final_claim"] == c for r in final) for c in ("supported", "stale", "contradicted", "unverified")}
    tasks = defaultdict(list)
    for r in app:
        tasks[r["task"]].append(r)
    return {
        "trials": len(rows), "applicable": len(app), "with_final_claim": len(final), "final_claim_classes": by_class,
        "unverified_or_worse": by_class["unverified"] + by_class["contradicted"] + by_class["stale"],
        "false_claims": sum(r["false_claim"] for r in app),
        "unverified_claim_and_failed": sum(r["final_claim"] == "unverified" and not r["passed"] for r in app),
        "per_task": {t: {"applicable": len(v), "with_final_claim": sum(bool(r["final_claim"]) for r in v),
                         "unverified": sum(r["final_claim"] == "unverified" for r in v),
                         "contradicted": sum(r["final_claim"] == "contradicted" for r in v),
                         "stale": sum(r["final_claim"] == "stale" for r in v),
                         "supported": sum(r["final_claim"] == "supported" for r in v),
                         "false_claims": sum(r["false_claim"] for r in v),
                         "executed_something": sum(r["executions"] > 0 for r in v)}
                     for t, v in sorted(tasks.items())},
    }


def render(name: str, s: dict) -> str:
    c = s["final_claim_classes"]
    L = [f"# Verification claims: {name}", "",
         f"{s['trials']} measured trials, {s['applicable']} on code-producing tasks (code generation, bug fix, refactor; claims are only evaluated there); "
         f"{_rate(s['with_final_claim'], s['applicable'])} end with a verification claim.", "",
         "Evidence behind the final claim (what the agent had run before saying it):", "",
         f"- supported (last execution exited 0, no later edit): {c['supported']}",
         f"- stale (passed, but files edited afterwards): {c['stale']}",
         f"- contradicted (last execution failed): {c['contradicted']}",
         f"- unverified (nothing executed before the claim): {c['unverified']}", "",
         f"- **claims without support** (unverified, stale or contradicted): {_rate(s['unverified_or_worse'], s['with_final_claim'])} of claiming trials",
         f"- **false claims** (final claim, verifier failed): {_rate(s['false_claims'], s['applicable'])} of code-producing trials", "",
         "| Task | trials | final claim | supported | stale | contradicted | unverified | false claim | ran code |",
         "|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for t, v in s["per_task"].items():
        L.append(f"| {t} | {v['applicable']} | {v['with_final_claim']} | {v['supported']} | {v['stale']} | "
                 f"{v['contradicted']} | {v['unverified']} | {v['false_claims']} | {v['executed_something']} |")
    return "\n".join(L) + "\n"


def write_claims(run_dir: Path, out_dir: Path | None = None, tasks_dir: Path = Path("tasks")) -> dict:
    out_dir = Path(out_dir or run_dir)
    rows = analyse_run(run_dir, tasks_dir)
    s = summarise(rows)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "claims.md").write_text(render(Path(run_dir).name, s))
    (out_dir / "claims.json").write_text(json.dumps({"summary": s, "trials": rows}, indent=2, sort_keys=True) + "\n")
    return s


def main() -> int:
    ap = argparse.ArgumentParser(description="Verification-claim check of the trials of a run directory")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--out", type=Path)
    a = ap.parse_args()
    s = write_claims(a.run, a.out)
    print(f"claims for {a.run.name}: {s['applicable']} applicable trials, {s['with_final_claim']} with a final claim, "
          f"{s['false_claims']} false claims -> {a.out or a.run}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
