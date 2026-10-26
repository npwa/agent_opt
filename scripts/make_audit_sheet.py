"""Blind audit sheet for the citation verifier's gold set: a stratified sample of citations from the full M6 answers.

120 citations from 15 answers (one per prompt, 8 per answer: 4 flagged and 4 supported where available), shuffled within an answer.
The sheet (AUDIT.md, labels.csv) does not show the verifier's verdict; key.json does. Fill the `real` column of labels.csv:
y = the cited work exists and says what the answer claims, p = exists but the details are wrong (authors, year, finding), n = cannot be found, u = unsure.

    python scripts/make_audit_sheet.py --run results/v1/m6_research --out results/v1/m6_audit
"""
import argparse
import csv
import json
import random
import re
from pathlib import Path

from harness import citations as ci


def context(answer: str, c: dict, width: int = 160) -> str:
    return re.sub(r"\s+", " ", answer[max(0, c["start"] - width): c["end"] + width]).strip()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--answers", type=int, default=15)
    ap.add_argument("--per-answer", type=int, default=8)
    a = ap.parse_args()
    rng = random.Random(7)
    by_task: dict[str, list] = {}
    for d in sorted((a.run / "trials").iterdir()):
        answer, evidence = ci.read_trial(d / "events.jsonl")
        if len(answer.split()) >= 200:
            res = ci.verify(ci.extract(answer), evidence)
            if len(res) >= 4:
                by_task.setdefault(d.name.split("__")[1], []).append((d.name, answer, res))
    tasks = sorted(by_task)
    rng.shuffle(tasks)
    items, key, n = [], {}, 0
    for t in tasks[: a.answers]:
        trial, answer, res = rng.choice(by_task[t])
        flagged = [c for c in res if c["verdict"] != "supported"]
        ok = [c for c in res if c["verdict"] == "supported"]
        rng.shuffle(flagged)
        rng.shuffle(ok)
        pick = flagged[: a.per_answer // 2] + ok[: a.per_answer // 2]
        pick += (flagged[a.per_answer // 2:] + ok[a.per_answer // 2:])[: a.per_answer - len(pick)]
        rng.shuffle(pick)
        for c in pick:
            n += 1
            items.append({"id": n, "trial": trial, "citation": c["raw"], "context": context(answer, c)})
            key[n] = {"verdict": c["verdict"], "why": c["why"]}
    a.out.mkdir(parents=True, exist_ok=True)
    with open(a.out / "labels.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id", "trial", "citation", "real", "note"])
        w.writerows([[i["id"], i["trial"], i["citation"], "", ""] for i in items])
    L = ["# Citation audit sheet (blind)", "", "For each item: does the cited work exist and say what the answer claims? Put `y` / `p` / `n` / `u` in "
         "`labels.csv` (y = yes, p = exists but details wrong, n = cannot be found, u = unsure). Context is quoted from the model's answer.", ""]
    cur = None
    for i in items:
        if i["trial"] != cur:
            cur = i["trial"]
            L += ["", f"## {cur}", ""]
        L.append(f"**{i['id']}.** `{i['citation']}`  ")
        L.append(f"> {i['context']}")
        L.append("")
    (a.out / "AUDIT.md").write_text("\n".join(L) + "\n")
    (a.out / "key.json").write_text(json.dumps(key, indent=1) + "\n")
    print(n, "items from", len({i["trial"] for i in items}), "answers;", sum(v["verdict"] != "supported" for v in key.values()), "flagged")


if __name__ == "__main__":
    main()
