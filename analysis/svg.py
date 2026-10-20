"""
analysis/svg.py

Dependency-free inline-SVG charts for the report (plan F14). Deterministic output (no ids,
timestamps or randomness) so the replay test can compare reports byte for byte.

Design rules (data-viz method): thin marks, 4 px rounded data-ends anchored to a square baseline,
2 px surface gaps between touching fills, 2 px surface ring on dots, hairline solid recessive
grid, categorical colours by fixed slot (never by rank), text in text tokens (never the series
colour), labels only where they fit, every mark carries a <title> (native tooltip) and every
chart has a table twin in the report. Colours come from CSS variables (--series-N, --ink, ...)
defined by the page, which is what makes the dark theme a selection and not a flip.
"""

from html import escape
from math import floor, log10

FONT = 11
CHAR_W = 0.56  # average glyph width / font size for the system sans (conservative)
SEGMENT_KEYS = ("prefill", "decode", "tool", "harness", "unattributed")
SEGMENT_CLASS = {"prefill": "s1", "decode": "s2", "tool": "s3", "harness": "s4", "unattributed": "sx"}


def esc(s: object) -> str:
    return escape(str(s), quote=True)


def num(v: float, d: int = 1) -> str:
    """Fixed-precision number without a negative zero (stable text for byte-exact reports)."""
    s = f"{v:.{d}f}"
    return s[1:] if s.startswith("-") and float(s) == 0 else s


def text_w(s: str, size: float = FONT) -> float:
    return len(s) * size * CHAR_W


def nice_ticks(lo: float, hi: float, target: int = 5) -> list[float]:
    """Round tick values covering [lo, hi]."""
    if hi <= lo:
        hi = lo + 1
    raw = (hi - lo) / max(1, target)
    mag = 10 ** floor(log10(raw))
    step = next(m * mag for m in (1, 2, 2.5, 5, 10) if m * mag >= raw)
    start = floor(lo / step) * step
    ticks, v = [], start
    while v <= hi + step * 0.999:
        ticks.append(round(v, 10))
        if v >= hi - 1e-12:
            break
        v += step
    return ticks


def compact(v: float) -> str:
    if abs(v) >= 1000:
        return f"{v / 1000:g}K"
    return f"{v:g}"


def _svg(w: float, h: float, label: str, body: list[str]) -> str:
    return (f'<svg class="chart" viewBox="0 0 {num(w, 0)} {num(h, 0)}" width="100%" role="img" '
            f'aria-label="{esc(label)}" xmlns="http://www.w3.org/2000/svg">' + "".join(body) + "</svg>")


def _x_axis(body: list[str], x0: float, x1: float, ybase: float, top: float, ticks: list[float],
            scale, fmt) -> None:
    for t in ticks:
        x = scale(t)
        body.append(f'<line class="grid" x1="{num(x)}" x2="{num(x)}" y1="{num(top)}" y2="{num(ybase)}"/>')
        body.append(f'<text class="tick" x="{num(x)}" y="{num(ybase + 15)}" text-anchor="middle">'
                    f"{esc(fmt(t))}</text>")
    body.append(f'<line class="axis" x1="{num(x0)}" x2="{num(x1)}" y1="{num(ybase)}" y2="{num(ybase)}"/>')


def _round_right_rect(x: float, y: float, w: float, h: float, r: float, cls: str, title: str) -> str:
    """Rect with a rounded right (data) end and a square left (baseline) end."""
    r = min(r, w, h / 2)
    d = (f"M{num(x, 2)},{num(y, 2)}H{num(x + w - r, 2)}a{num(r, 2)},{num(r, 2)} 0 0 1 {num(r, 2)},{num(r, 2)}"
         f"V{num(y + h - r, 2)}a{num(r, 2)},{num(r, 2)} 0 0 1 {num(-r, 2)},{num(r, 2)}H{num(x, 2)}Z")
    return f'<path class="{cls}" d="{d}"><title>{esc(title)}</title></path>'


# ---- dot + whisker per category --------------------------------------------------------------------------

def dot_whisker(rows: list[dict], n_series: int, x_label: str, fmt, x_max: float | None = None,
                label: str = "") -> str:
    """rows: [{label, points: [{series: int, v, lo, hi, title}]}]; series -> colour slot s1..sN."""
    label_w = min(260, max([text_w(r["label"]) for r in rows] + [60]) + 12)
    left, right, top, bottom, plot_w = label_w, 24, 8, 36, 600
    row_h = 14 + 14 * max(1, n_series)
    height = top + row_h * len(rows) + bottom
    hi_all = max([p["hi"] for r in rows for p in r["points"]] + [0.0])
    if x_max is None:  # auto: round the axis up to a clean tick
        x_max = nice_ticks(0, hi_all)[-1] if hi_all > 0 else 1.0
    ticks = [t for t in nice_ticks(0, x_max) if t <= x_max + 1e-9]

    def sx(v: float) -> float:
        return left + plot_w * v / x_max

    body: list[str] = []
    _x_axis(body, left, left + plot_w, top + row_h * len(rows), top, ticks, sx, fmt)
    for i, r in enumerate(rows):
        y0 = top + i * row_h
        body.append(f'<text class="rowlabel" x="{num(left - 10)}" y="{num(y0 + row_h / 2 + 4)}" '
                    f'text-anchor="end">{esc(r["label"])}</text>')
        for p in r["points"]:
            y = y0 + 12 + p["series"] * 14 + 7
            cls = f"s{p['series'] + 1}"
            x_lo, x_hi, x_v = sx(max(0, p["lo"])), sx(min(x_max, p["hi"])), sx(min(x_max, p["v"]))
            body.append(f'<g class="pt {cls}"><title>{esc(p["title"])}</title>'
                        f'<line class="whisker" x1="{num(x_lo)}" x2="{num(x_hi)}" y1="{num(y)}" y2="{num(y)}"/>'
                        f'<line class="whisker" x1="{num(x_lo)}" x2="{num(x_lo)}" y1="{num(y - 3)}" y2="{num(y + 3)}"/>'
                        f'<line class="whisker" x1="{num(x_hi)}" x2="{num(x_hi)}" y1="{num(y - 3)}" y2="{num(y + 3)}"/>'
                        f'<circle class="dot" cx="{num(x_v)}" cy="{num(y)}" r="4.5"/></g>')
    body.append(f'<text class="axislabel" x="{num(left + plot_w / 2)}" y="{num(height - 4)}" '
                f'text-anchor="middle">{esc(x_label)}</text>')
    return _svg(left + plot_w + right, height, label or x_label, body)


# ---- stacked horizontal bars (time decomposition) --------------------------------------------------------

def stacked_bars(rows: list[dict], label: str = "") -> str:
    """rows: [{label, total_text, segments: {key: pct}, titles: {key: str}}]; pct sums to ~100."""
    label_w = min(220, max([text_w(r["label"]) for r in rows] + [60]) + 12)
    right_w, top, bottom, plot_w, bar_h, row_h, gap = 150, 8, 36, 560, 24, 40, 2
    height = top + row_h * len(rows) + bottom
    scale = lambda p: label_w + plot_w * p / 100.0  # noqa: E731
    body: list[str] = []
    _x_axis(body, label_w, label_w + plot_w, top + row_h * len(rows), top, [0, 25, 50, 75, 100], scale,
            lambda t: f"{t:g}%")
    for i, r in enumerate(rows):
        y = top + i * row_h + (row_h - bar_h) / 2
        body.append(f'<text class="rowlabel" x="{num(label_w - 10)}" y="{num(top + i * row_h + row_h / 2 + 4)}" '
                    f'text-anchor="end">{esc(r["label"])}</text>')
        segs = [(k, r["segments"].get(k, 0.0)) for k in SEGMENT_KEYS if r["segments"].get(k, 0.0) > 0.05]
        x = label_w
        for j, (k, pct) in enumerate(segs):
            w_full = plot_w * pct / 100.0
            last = j == len(segs) - 1
            w = w_full if last else max(0.0, w_full - gap)  # 2 px surface gap between fills
            title = r["titles"].get(k, f"{k}: {pct:.1f}%")
            cls = SEGMENT_CLASS[k]
            if w >= 1:
                if last:
                    body.append(_round_right_rect(x, y, w, bar_h, 4, f"seg {cls}", title))
                else:
                    body.append(f'<rect class="seg {cls}" x="{num(x, 2)}" y="{num(y, 2)}" width="{num(w, 2)}" '
                                f'height="{bar_h}"><title>{esc(title)}</title></rect>')
                txt = f"{pct:.0f}%"
                if w >= text_w(txt) + 12:  # label only if it fits with padding on both sides
                    body.append(f'<text class="seglabel on-{cls}" x="{num(x + w / 2)}" '
                                f'y="{num(y + bar_h / 2 + 4)}" text-anchor="middle">{txt}</text>')
            x += w_full
        body.append(f'<text class="tick" x="{num(label_w + plot_w + 12)}" y="{num(y + bar_h / 2 + 4)}">'
                    f'{esc(r["total_text"])}</text>')
    body.append(f'<text class="axislabel" x="{num(label_w + plot_w / 2)}" y="{num(height - 4)}" '
                f'text-anchor="middle">share of wall-clock time</text>')
    return _svg(label_w + plot_w + right_w, height, label or "Time decomposition", body)


# ---- Pareto scatter (success vs time) --------------------------------------------------------------------

def pareto_front(points: list[dict]) -> list[bool]:
    """True for points not dominated by another (higher success AND lower-or-equal time, or
    equal success and strictly lower time)."""
    flags = []
    for i, p in enumerate(points):
        dominated = any(
            (q["y"] > p["y"] and q["x"] <= p["x"]) or (q["y"] >= p["y"] and q["x"] < p["x"])
            for j, q in enumerate(points) if j != i)
        flags.append(not dominated)
    return flags


def pareto(points: list[dict], x_label: str = "median wall-clock time per trial (s)",
           y_label: str = "success rate", label: str = "") -> str:
    """points: [{label, x, y (0..1), x_lo, x_hi, y_lo, y_hi, title}] - one per configuration."""
    left, right, top, bottom, pw, ph = 56, 120, 12, 44, 640, 300
    x_hi_all = max([p["x_hi"] for p in points] + [1.0])
    ticks = nice_ticks(0, x_hi_all)
    x_max = ticks[-1]
    sx = lambda v: left + pw * v / x_max  # noqa: E731
    sy = lambda v: top + ph * (1 - v)  # noqa: E731
    body: list[str] = []
    _x_axis(body, left, left + pw, top + ph, top, ticks, sx, compact)
    for t in (0, 0.25, 0.5, 0.75, 1.0):
        body.append(f'<line class="grid" x1="{left}" x2="{left + pw}" y1="{num(sy(t))}" y2="{num(sy(t))}"/>')
        body.append(f'<text class="tick" x="{left - 8}" y="{num(sy(t) + 4)}" text-anchor="end">{int(t * 100)}%</text>')
    flags = pareto_front(points)
    front = sorted((p for p, f in zip(points, flags, strict=True) if f), key=lambda p: (p["x"], p["y"]))
    if len(front) > 1:  # stepped frontier: run right at the current success rate, then step up
        d = f"M{num(sx(front[0]['x']))},{num(sy(front[0]['y']))}"
        for b in front[1:]:
            d += f"H{num(sx(b['x']))}V{num(sy(b['y']))}"
        body.append(f'<path class="frontier" d="{d}"/>')
    # occupied boxes: whiskers and the frontier, so direct labels never sit on a line
    placed: list[tuple[float, float, float, float]] = []
    for p in points:
        cx, cy = sx(p["x"]), sy(p["y"])
        placed.append((sx(p["x_lo"]), cy - 2, sx(p["x_hi"]), cy + 2))
        placed.append((cx - 2, sy(p["y_hi"]), cx + 2, sy(p["y_lo"])))
    for a_, b_ in zip(front, front[1:], strict=False):
        placed.append((sx(a_["x"]), sy(a_["y"]) - 2, sx(b_["x"]), sy(a_["y"]) + 2))
        placed.append((sx(b_["x"]) - 2, sy(b_["y"]), sx(b_["x"]) + 2, sy(a_["y"])))
    for p, f in zip(points, flags, strict=True):
        cx, cy = sx(p["x"]), sy(p["y"])
        cls = "front" if f else "dominated"
        body.append(f'<g class="pt {cls}"><title>{esc(p["title"])}</title>'
                    f'<line class="whisker2" x1="{num(sx(p["x_lo"]))}" x2="{num(sx(p["x_hi"]))}" y1="{num(cy)}" y2="{num(cy)}"/>'
                    f'<line class="whisker2" x1="{num(cx)}" x2="{num(cx)}" y1="{num(sy(p["y_lo"]))}" y2="{num(sy(p["y_hi"]))}"/>'
                    f'<circle class="dot" cx="{num(cx)}" cy="{num(cy)}" r="{6 if f else 5}"/></g>')
    labels: list[tuple[float, float, float, float]] = []
    for p in points:
        cx, cy = sx(p["x"]), sy(p["y"])
        w, h = text_w(p["label"]), FONT + 2
        spot = None
        # first free corner position (above/below, right/left of the dot), else the last candidate
        for dx, dy, anchor in ((9, -9, "start"), (9, 17, "start"), (-9, -9, "end"), (-9, 17, "end"),
                               (12, 4, "start"), (-12, 4, "end")):
            lx = cx + dx
            x0 = lx if anchor == "start" else lx - w
            box = (x0, cy + dy - h + 3, x0 + w, cy + dy + 3)
            spot = (lx, cy + dy, anchor, box)
            if not any(box[0] < o[2] and box[2] > o[0] and box[1] < o[3] and box[3] > o[1]
                       for o in placed + labels):
                break
        labels.append(spot[3])
        body.append(f'<text class="pointlabel" x="{num(spot[0])}" y="{num(spot[1])}" '
                    f'text-anchor="{spot[2]}">{esc(p["label"])}</text>')
    body.append(f'<text class="axislabel" x="{num(left + pw / 2)}" y="{num(top + ph + 38)}" '
                f'text-anchor="middle">{esc(x_label)}</text>')
    body.append(f'<text class="axislabel" transform="translate(14,{num(top + ph / 2)}) rotate(-90)" '
                f'text-anchor="middle">{esc(y_label)}</text>')
    return _svg(left + pw + right, top + ph + bottom, label or "Success versus time", body)
