"""Generator of fixtures/tasks/longctx_11_harbor_report/report.md (deterministic; run: python3 make_report.py <out dir>).

A long annual report in which the body states some figures that an errata appendix later corrects. The questions of the
task can only be answered correctly by reading to the end and applying the corrections. facts.json (the ground truth)
is written next to the report for tests; it is not part of the fixture the agent sees.
"""

import json
import random
import sys
from pathlib import Path

# name: (throughput FY2025 kTEU as printed in the body, incidents as printed, downtime FY2025 as printed, downtime FY2024)
TERMINALS = {
    "Alder Point": (318, 9, 82, 88),
    "Brightwater": (276, 14, 133, 120),
    "Cormorant Quay": (354, 12, 101, 95),
    "Dunmore": (197, 11, 138, 150),
    "East Spit": (262, 7, 70, 64),
    "Fennick": (412, 13, 95, 110),
    "Garrow Bay": (289, 14, 124, 130),
    "Holloway": (231, 10, 120, 99),
    "Ironbridge": (305, 11, 120, 141),
}
CORRECTIONS = {  # (terminal, field): corrected value
    ("Fennick", "throughput"): 441,
    ("Garrow Bay", "incidents"): 31,
    ("Ironbridge", "downtime"): 160,
}
STAFF = {"Alder Point": 212, "Brightwater": 198, "Cormorant Quay": 240, "Dunmore": 143, "East Spit": 176, "Fennick": 265,
         "Garrow Bay": 188, "Holloway": 159, "Ironbridge": 201}

TOPICS = ["berth scheduling", "crane maintenance cycles", "yard reconfiguration", "gate automation", "customer liaison",
          "pilotage coordination", "lighting upgrades", "training programmes", "rail-link planning", "shore-power trials",
          "container tracking", "waste handling", "stakeholder meetings", "dredging windows", "security screening"]
VERBS = ["reviewed", "rescheduled", "piloted", "extended", "documented", "consolidated", "audited", "simplified"]
NOUNS = ["working groups", "supplier workshops", "inspection rounds", "pilot schemes", "joint exercises", "survey waves"]
WEEKDAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]


def filler(rng: random.Random, n: int) -> str:
    out = []
    for _ in range(n):
        t = rng.choice(TOPICS)
        a, b = rng.sample(NOUNS, 2)
        out.append(
            f"During the year the {t} team {rng.choice(VERBS)} its routines through {rng.randint(3, 19)} {a} and "
            f"{rng.randint(2, 11)} {b}, mostly on {rng.choice(WEEKDAYS)}s. Staff feedback scored {rng.randint(61, 94)} out of 100, "
            f"and the cost of the programme stayed within {rng.randint(2, 9)}.{rng.randint(0, 9)} percent of plan.\n"
            f"A follow-up is scheduled for the {rng.choice(['first', 'second', 'third', 'fourth'])} quarter, subject to the "
            f"availability of {rng.choice(['pilots', 'tugs', 'inspectors', 'planners'])}.\n")
    return "\n".join(out)  # one sentence pair per line: long lines would be cut by the read tool


def corrected(name: str) -> dict:
    thr, inc, dt25, dt24 = TERMINALS[name]
    return {"throughput": CORRECTIONS.get((name, "throughput"), thr), "incidents": CORRECTIONS.get((name, "incidents"), inc),
            "downtime": CORRECTIONS.get((name, "downtime"), dt25), "downtime_2024": dt24}


def facts() -> dict:
    rows = {n: corrected(n) for n in TERMINALS}
    for r in rows.values():
        r["rate"] = r["incidents"] / r["throughput"] * 100
    total_thr = sum(r["throughput"] for r in rows.values())
    d25, d24 = sum(r["downtime"] for r in rows.values()), sum(r["downtime_2024"] for r in rows.values())
    top = max(rows, key=lambda n: rows[n]["rate"])
    green = [n for n, r in rows.items() if r["rate"] < 4.0 and r["downtime"] < 140 and r["throughput"] >= 250]
    return {"total_throughput_ktueu": total_thr, "highest_rate_terminal": top, "highest_rate": round(rows[top]["rate"], 2),
            "downtime_change_pct": round((d25 - d24) / d24 * 100, 1), "downtime_2024": d24, "downtime_2025": d25,
            "green_berth": green, "approver": "Board of Directors", "approval_date": "21 November 2025",
            "pilot_rule_date": "1 April 2026",
            "uncorrected": {"total_throughput": sum(v[0] for v in TERMINALS.values()),
                            "downtime_2025": sum(v[2] for v in TERMINALS.values())}}


def build(seed: int = 11) -> str:
    rng = random.Random(seed)
    L = ["# Harbor Authority of Marrow Sound: Annual Operations Report FY2025", "",
         "*Prepared by the Office of Operations Analysis. Corrections that were issued after printing are collected in "
         "Appendix A. Where Appendix A conflicts with the body of the report, Appendix A prevails.*", "",
         "## 1. Overview", "", filler(rng, 7), "",
         "The nine terminals handled the volumes, incidents and downtime reported in sections 3.1 to 3.9. "
         "Section 7 defines the Green Berth recognition, section 8 the safety budget and section 9 the pilotage rule.", ""]
    for i in range(2, 7):
        names = ["Governance and reporting", "Workforce and training", "Environmental programme", "Customer relations",
                 "Capital projects"]
        L += [f"## 2.{i - 1} {names[i - 2]}", "", filler(rng, 8), ""]
    L += ["## 3. Terminal performance", "", "Each subsection gives throughput in thousand TEU (kTEU), the number of reportable "
          "incidents, unplanned downtime in hours (FY2024 in brackets) and staff numbers.", ""]
    for k, (name, (thr, inc, dt25, dt24)) in enumerate(TERMINALS.items(), 1):
        L += [f"### 3.{k} {name}", "",
              f"Throughput in FY2025 was {thr} kTEU. The terminal recorded {inc} reportable incidents. Unplanned downtime "
              f"totalled {dt25} hours (FY2024: {dt24} hours). Staff numbered {STAFF[name]}.", "", filler(rng, 7), ""]
    L += ["## 4. Financial summary", "", filler(rng, 6), "",
          "## 5. Fleet and equipment", "", filler(rng, 7), "",
          "## 6. Risk register", "", filler(rng, 6), "",
          "## 7. Green Berth recognition", "",
          "A terminal receives the Green Berth recognition for FY2025 if all three conditions hold: its incident rate is below "
          "4.0 reportable incidents per 100 kTEU, its unplanned downtime is below 140 hours, and its throughput is at least "
          "250 kTEU. The incident rate is the number of reportable incidents divided by throughput in kTEU, times 100.", "",
          filler(rng, 5), "",
          "## 8. Safety budget", "",
          "The revised safety budget of EUR 4.2 million was approved on 14 November 2025 by the Chief Financial Officer, "
          "Lena Moravec. Spending is released in four tranches.", "", filler(rng, 5), "",
          "## 9. Pilotage rule", "",
          "The new pilotage rule for vessels above 9,000 TEU takes effect on 1 March 2026. Masters have been informed by "
          "circular.", "", filler(rng, 5), "",
          "## 10. Outlook", "", filler(rng, 6), "",
          "## Appendix A: Errata (these corrections supersede the body of the report)", "",
          "1. Section 3.6 (Fennick): throughput in FY2025 was 441 kTEU, not 412 kTEU.",
          "2. Section 3.7 (Garrow Bay): the terminal recorded 31 reportable incidents, not 14.",
          "3. Section 3.9 (Ironbridge): unplanned downtime in FY2025 totalled 160 hours, not 120 hours.",
          "4. Section 8: the revised safety budget was approved by the Board of Directors on 21 November 2025. The Chief "
          "Financial Officer's sign-off on 14 November 2025 was a recommendation only.",
          "5. Section 9: the pilotage rule takes effect on 1 April 2026, not on 1 March 2026.", "",
          "## Appendix B: Superseded draft figures (kept for the record, not to be used)", "",
          "The first draft of this report listed a total throughput of 2,571 kTEU, a total downtime of 951 hours for FY2025 "
          "and a pilotage rule effective 1 January 2026. These figures were withdrawn before printing.", "",
          filler(rng, 6), ""]
    return "\n".join(L)


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    out.mkdir(parents=True, exist_ok=True)
    text = build()
    (out / "report.md").write_text(text)
    (out / "facts.json").write_text(json.dumps(facts(), indent=2) + "\n")
    print(f"{len(text)} characters, about {len(text) // 4} tokens")
