#!/usr/bin/env python3
"""Generate the long-context fixture report.md (~12k tokens) deterministically.

Six target facts are buried at fixed relative positions among generated filler;
the task rubric checks the summary for them. Run: make_report.py <outfile>
"""
import random
import sys
from pathlib import Path

SEED = 7
SECTIONS = 24

FACTS = {  # section index -> paragraph
    3: "Turbine T-4 was taken offline on 14 March 2025 after the bearing seal corroded through, "
       "allowing seawater into the nacelle. This was the only failure in the pilot that caused a "
       "multi-week outage, and the root cause was traced to corrosion of the bearing seal housing.",
    8: "Over the full 24-month operating period the pilot delivered 18.7 GWh to the grid, "
       "which was 6 percent below the 19.9 GWh forecast in the original business case.",
    12: "Final project cost came to EUR 41.25 million against an approved budget of EUR 36.7 million, "
        "a budget overrun of EUR 4.55 million (12.4 percent), driven mainly by the T-4 repair campaign.",
    16: "Lead engineer Dr. Ines Marchetti recommended replacing the titanium fastener set with duplex "
        "stainless steel on all remaining nacelles, a change the steering group accepted in principle.",
    20: "The independent fish-monitoring programme compared migration counts before and after "
        "installation and found no statistically significant change (p = 0.41).",
    23: "Decision: the steering group approved Phase 2 for the third quarter of 2026, contingent on "
        "a completed redesign of the bearing seal and a successful dry-dock endurance test.",
}

TOPICS = ["Site conditions", "Mooring and foundations", "Grid connection", "Maintenance logistics",
          "Vessel scheduling", "Data acquisition", "Supply chain", "Permitting", "Community engagement",
          "Safety performance", "Insurance", "Spare parts", "Training", "Weather windows",
          "Software and controls", "Calibration"]
OPEN = ["During the reporting period the team reviewed", "The working group examined",
        "Operations staff documented", "An internal audit looked at", "The quarterly review covered"]
MID = ["with attention to routine procedures", "and found the process broadly stable",
       "noting several minor administrative corrections", "and recorded observations for later study",
       "without identifying a material concern"]
DETAIL = ["Records were kept in the shared logbook and reconciled monthly.",
          "Two small deviations were logged and closed within the week.",
          "The contractor supplied the required documentation on schedule.",
          "Sensor readings stayed inside the expected envelope for most of the period.",
          "A handover meeting was held and minutes were circulated to all participants.",
          "Minor delays were attributed to sea state and were recovered later.",
          "Turbine T-2 had a short software-related restart in August 2024 that was resolved remotely.",
          "Turbine T-1 and T-3 ran without unplanned interruption in the quarter.",
          "The cost of this item was within the contingency allowance.",
          "Training refreshers were completed by the offshore crew."]


def para(rnd: random.Random) -> str:
    topic = rnd.choice(TOPICS).lower()
    s = [f"{rnd.choice(OPEN)} {topic} {rnd.choice(MID)}."]
    s += rnd.sample(DETAIL, k=rnd.randint(4, 7))
    s.append(f"Figure {rnd.randint(2, 98)} of the appendix lists the {rnd.randint(11, 63)} items reviewed.")
    return " ".join(s)


def main(out: str) -> None:
    rnd = random.Random(SEED)
    parts = ["# Meridian Tidal Pilot - Final Operations Review", ""]
    for i in range(SECTIONS):
        parts.append(f"## {i + 1}. {rnd.choice(TOPICS)} (period {i % 8 + 1})")
        parts.append("")
        for _ in range(rnd.randint(3, 4)):
            parts.append(para(rnd))
            parts.append("")
        if i in FACTS:
            parts.insert(len(parts) - rnd.randint(2, 6), FACTS[i] + "\n")
    Path(out).write_text("\n".join(parts))


if __name__ == "__main__":
    main(sys.argv[1])
