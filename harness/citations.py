"""
harness/citations.py

Citation verifier (plan item M6, F10): does a research answer cite only what its own search session retrieved?

1. ``extract`` finds citation-like items in the final answer with conservative patterns: author-year (``Soon et al. (2008)``,
   ``Haynes & Rees, 2011``, ``Libet, Gleason, Wright (1983)``), URLs and DOIs.
2. The evidence is the output of every completed tool call of the trial (search results, fetched pages), never the prompt.
3. ``verify`` gives each citation a verdict: ``supported`` (first-author surname and year occur close together in the evidence and every
   further named author occurs in it), ``partial`` (the surname is there but the year is not near it, or a co-author is missing: the
   "invented co-authors" case) or ``unsupported`` (the surname never appears). URLs and DOIs are supported when they occur in the evidence.
4. ``gate`` marks (or drops) the flagged citations in the answer: the post-hoc gate of the evaluation.

    python -m harness.citations --run results/v1/m6_research
"""

import argparse
import json
import re
import unicodedata
from pathlib import Path

from analysis import stats

NAME = r"[A-Z][A-Za-z'’]{2,}(?:-[A-Z]?[A-Za-z'’]+)*"
YEAR = r"(?:19|20)\d\d"
STOP = {"the", "in", "since", "during", "from", "after", "before", "by", "as", "for", "over", "around", "between", "until", "early", "late",
        "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december",
        "fall", "spring", "summer", "winter", "figure", "table", "section", "version", "phase", "grade", "level", "year", "covid", "model",
        "mid", "under", "about", "while", "when", "with", "than", "that", "this", "these", "those", "and", "also", "but", "via", "per", "circa"}
KIND = r"(?:\s+(?:debate|study|paper|trial|review|experiment|results|findings|work|analysis|meta-analysis|report|studies))?"
AUTHORS = re.compile(rf"\b({NAME})((?:\s*(?:,\s*(?:and\s+|&\s*)?|and\s+|&\s*){NAME})*)(\s+et al\.?)?{KIND}\s*[,(]?\s*\(?({YEAR})(?!s\b)[a-d]?(?:[–-]{YEAR})?\)?")
URL = re.compile(r"https?://[^\s)\]>\"'`]+[^\s)\]>\"'`.,;:]")
DOI = re.compile(r"\b10\.\d{4,9}/[^\s)\]>\"'`]+[^\s)\]>\"'`.,;:]")
NEAR = 400  # characters between a surname and its year in the evidence


def fold(s: str) -> str:
    return unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode().lower()


def extract(text: str) -> list[dict]:
    """Citation-like items: {"kind": "author_year"|"url"|"doi", "raw", "start", "end", ...}."""
    out = []
    for m in AUTHORS.finditer(text):
        first = m.group(1)
        if first.lower() in STOP:
            continue
        others = [n for n in re.findall(NAME, m.group(2) or "") if n.lower() not in STOP]
        out.append({"kind": "author_year", "raw": m.group(0), "start": m.start(), "end": m.end(),
                    "first": first, "others": others, "year": m.group(4)})
    urls = [(m.start(), m.end()) for m in URL.finditer(text)]
    for rx, kind in ((URL, "url"), (DOI, "doi")):
        for m in rx.finditer(text):
            if kind == "doi" and any(a <= m.start() and m.end() <= b for a, b in urls):
                continue  # a doi.org link is one citation, not two
            out.append({"kind": kind, "raw": m.group(0), "start": m.start(), "end": m.end()})
    return sorted(out, key=lambda c: c["start"])


def norm_url(u: str) -> str:
    return re.sub(r"^https?://(www\.)?", "", fold(u)).split("#")[0].rstrip("/")


def verify(citations: list[dict], evidence: str) -> list[dict]:
    ev = fold(evidence)
    res = []
    for c in citations:
        c = dict(c)
        if c["kind"] == "author_year":
            sur = fold(c["first"])
            hits = [m.start() for m in re.finditer(rf"\b{re.escape(sur)}\b", ev)]
            year_near = any(c["year"] in ev[max(0, h - NEAR): h + NEAR] for h in hits)
            missing = [o for o in c["others"] if not re.search(rf"\b{re.escape(fold(o))}\b", ev)]
            if not hits:
                c["verdict"], c["why"] = "unsupported", "surname not in the retrieved pages"
            elif not year_near:
                c["verdict"], c["why"] = "partial", "surname found, year not near it"
            elif missing:
                c["verdict"], c["why"] = "partial", "co-author not in the retrieved pages: " + ", ".join(missing)
            else:
                c["verdict"], c["why"] = "supported", ""
        else:
            key = norm_url(c["raw"]) if c["kind"] == "url" else fold(c["raw"])
            ok = key in ev
            c["verdict"], c["why"] = ("supported", "") if ok else ("unsupported", f"{c['kind']} not in the retrieved pages")
        res.append(c)
    return res


def gate(text: str, results: list[dict], mode: str = "mark") -> str:
    """Post-hoc gate: flagged citations are marked ``[unverified]`` (mode "mark") or removed (mode "drop")."""
    out, last = [], 0
    for c in sorted(results, key=lambda c: c["start"]):
        if c["verdict"] == "supported" or c["start"] < last:
            continue
        out.append(text[last:c["start"]])
        out.append(c["raw"] + " [unverified]" if mode == "mark" else "")
        last = c["end"]
    out.append(text[last:])
    return "".join(out)


# ---- trials --------------------------------------------------------------------------------------------------------

def read_trial(events: Path) -> tuple[str, str]:
    """(final answer, evidence): the text after the last tool call (fallback: the last text), and all tool outputs."""
    steps, evidence = [], []
    for line in Path(events).read_text(errors="replace").splitlines():
        try:
            e = json.loads(line)
        except json.JSONDecodeError:
            continue
        part = e.get("part") or {}
        if e.get("type") == "text" and part.get("text"):
            steps.append(("text", part["text"]))
        elif e.get("type") == "tool_use":
            st = part.get("state") or {}
            if st.get("status") == "completed" and isinstance(st.get("output"), str):
                evidence.append(st["output"])
            steps.append(("tool", ""))
    last_tool = max((i for i, s in enumerate(steps) if s[0] == "tool"), default=-1)
    tail = [t for k, t in steps[last_tool + 1:] if k == "text"]
    if not tail:
        texts = [t for k, t in steps if k == "text"]
        tail = texts[-1:]
    return "\n".join(tail), "\n".join(evidence)


def analyse_run(run: Path) -> list[dict]:
    rows = []
    for d in sorted((Path(run) / "trials").iterdir()):
        ev = d / "events.jsonl"
        if not ev.exists():
            continue
        rec = json.loads((d / "record.json").read_text()) if (d / "record.json").exists() else {}
        answer, evidence = read_trial(ev)
        res = verify(extract(answer), evidence)
        rows.append({"trial": d.name, "task": rec.get("task", d.name), "status": rec.get("status"), "words": len(answer.split()),
                     "citations": res, "n": len(res), "flagged": sum(c["verdict"] != "supported" for c in res)})
    return rows


def render(rows: list[dict]) -> str:
    n = sum(r["n"] for r in rows)
    f = sum(r["flagged"] for r in rows)
    lo, hi = stats.wilson_interval(f, n) if n else (0, 0)
    L = ["# Citation check", "", f"{len(rows)} trials, {n} citation-like items in the final answers, **{f} flagged** "
         f"({f / n:.0%}, 95% Wilson {lo:.0%}-{hi:.0%}) - not supported by the pages the trial retrieved." if n else "No citations found.", "",
         "| Trial | words | citations | supported | partial | unsupported |", "|---|---:|---:|---:|---:|---:|"]
    for r in rows:
        v = [sum(c["verdict"] == k for c in r["citations"]) for k in ("supported", "partial", "unsupported")]
        L.append(f"| {r['trial']} | {r['words']} | {r['n']} | {v[0]} | {v[1]} | {v[2]} |")
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Check the citations in research answers against the retrieved pages")
    ap.add_argument("--run", type=Path, required=True)
    a = ap.parse_args()
    rows = analyse_run(a.run)
    (a.run / "citations.json").write_text(json.dumps(rows, indent=1) + "\n")
    (a.run / "citations.md").write_text(render(rows))
    print(render(rows))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
