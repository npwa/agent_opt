"""
analysis/figures.py

Standalone SVG figures (for README / slides) from a run's summary.json. Same chart code and
colours as report.html, with the page CSS embedded so the files render on their own.

    python -m analysis.figures --run results/v1/baseline    # -> <run>/figures/*.svg
"""

import argparse
import json
import re
from pathlib import Path

from analysis import svg
from analysis.report import CSS, SEG_LABEL


def _with_legend(chart: str, items: list[tuple[str, str]]) -> str:
    """Append a one-row legend (colour swatch + label) below the chart and grow the viewBox."""
    m = re.search(r'viewBox="0 0 (\d+) (\d+)"', chart)
    w, h = int(m.group(1)), int(m.group(2))
    parts, x = [], 12.0
    for cls, label in items:
        parts.append(f'<rect class="seg {cls}" x="{x:.0f}" y="{h + 6}" width="12" height="12" rx="2"/>'
                     f'<text x="{x + 18:.0f}" y="{h + 16}">{label}</text>')
        x += 18 + svg.text_w(label) + 22
    chart = chart.replace(m.group(0), f'viewBox="0 0 {w} {h + 28}"')
    return chart.replace("</svg>", "".join(parts) + "</svg>")


def _standalone(chart: str) -> str:
    """Embed the report CSS in the <svg>, give it a surface, and make it self-contained."""
    style = f"<style>{CSS}</style><rect width='100%' height='100%' fill='var(--surface)'/>"
    head, rest = chart.split(">", 1)
    return f'<?xml version="1.0" encoding="UTF-8"?>\n{head}>{style}{rest}\n'


def time_figure(s: dict, config: str) -> str:
    d = s["decomposition"][config]
    cats = sorted(d["categories"].items(), key=lambda kv: -kv[1]["pct"]["prefill"])
    rows = [{"label": name.replace("_", " "),
             "total_text": f"n={a['n']} · {a['wall_s'] / max(1, a['n']):.0f} s/trial",
             "segments": a["pct"],
             "titles": {k: f"{name}: {SEG_LABEL[k]} {a['pct'][k]:.1f}%" for k in a["pct"]}}
            for name, a in [*cats, ("all tasks", d["all"])]]
    chart = svg.stacked_bars(rows, f"Where the time goes, {config}")
    used = [k for k in svg.SEGMENT_KEYS if k != "unattributed"]
    return _standalone(_with_legend(chart, [(svg.SEGMENT_CLASS[k], SEG_LABEL[k]) for k in used]))


def pass_rate_figure(s: dict, config: str) -> str:
    cells = [(t, s["cells"][f"{config}|{t}"]) for t in s["tasks"] if f"{config}|{t}" in s["cells"]]
    cells.sort(key=lambda tc: (-tc[1]["rate"], tc[0]))
    rows = [{"label": t, "points": [{"series": 0, "v": c["rate"], "lo": c["wilson"][0], "hi": c["wilson"][1],
                                     "title": f"{t}: {c['passes']}/{c['n']} passed"}]} for t, c in cells]
    return _standalone(svg.dot_whisker(rows, 1, "pass rate (95% Wilson interval)",
                                       lambda v: f"{v * 100:.0f}%", 1.0, f"Pass rate per task, {config}"))


def write_figures(run_dir: Path, out_dir: Path | None = None) -> list[Path]:
    s = json.loads((run_dir / "summary.json").read_text())
    out_dir = Path(out_dir or run_dir / "figures")
    out_dir.mkdir(parents=True, exist_ok=True)
    config = s["configs"][0]
    written = []
    for name, text in (("time_by_task_type.svg", time_figure(s, config)),
                       ("pass_rate_by_task.svg", pass_rate_figure(s, config))):
        (out_dir / name).write_text(text)
        written.append(out_dir / name)
    return written


def main() -> int:
    ap = argparse.ArgumentParser(description="Write standalone SVG figures for a run directory")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--out", type=Path)
    a = ap.parse_args()
    for p in write_figures(a.run, a.out):
        print(p)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
