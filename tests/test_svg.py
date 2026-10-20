import xml.etree.ElementTree as ET

import pytest

from analysis import svg


def parse(s: str) -> ET.Element:
    return ET.fromstring(s)  # well-formed XML (the report embeds it inline)


def texts(root: ET.Element) -> list[str]:
    return ["".join(t.itertext()) for t in root.iter("{http://www.w3.org/2000/svg}text")]


def test_nice_ticks_cover_range_with_round_values():
    assert svg.nice_ticks(0, 100) == [0, 20, 40, 60, 80, 100]
    t = svg.nice_ticks(0, 87.5)
    assert t[0] == 0 and t[-1] >= 87.5 and all(round(v * 4) == v * 4 for v in t)
    assert svg.nice_ticks(0, 0)[-1] > 0  # degenerate range still yields an axis
    assert svg.compact(1500) == "1.5K" and svg.compact(40) == "40"


def test_num_never_prints_negative_zero_and_esc_escapes_quotes():
    assert svg.num(-0.0004, 1) == "0.0" and svg.num(-1.26, 1) == "-1.3"
    assert svg.esc('<b x="1">&') == "&lt;b x=&quot;1&quot;&gt;&amp;"


def test_pareto_front_logic():
    pts = [{"x": 10, "y": 0.5}, {"x": 20, "y": 0.9}, {"x": 30, "y": 0.9}, {"x": 15, "y": 0.4},
           {"x": 10, "y": 0.5}]
    # (30, .9) is dominated by (20, .9); (15, .4) by (10, .5); the two identical points both stay
    assert svg.pareto_front(pts) == [True, True, False, False, True]
    assert svg.pareto_front([{"x": 1, "y": 1}]) == [True]


def dot_rows(n=2):
    return [{"label": "task_a", "points": [{"series": i, "v": 0.5, "lo": 0.2, "hi": 0.8,
                                            "title": f"s{i} <b>"} for i in range(n)]},
            {"label": "task_b", "points": [{"series": 0, "v": 1.0, "lo": 0.7, "hi": 1.0, "title": "t"}]}]


def test_dot_whisker_is_valid_svg_with_titles_and_rings():
    root = parse(svg.dot_whisker(dot_rows(), 2, "rate", lambda v: f"{v:.0%}", 1.0, "chart"))
    assert root.attrib["role"] == "img" and root.attrib["aria-label"] == "chart"
    circles = list(root.iter("{http://www.w3.org/2000/svg}circle"))
    assert len(circles) == 3 and all(c.attrib["r"] == "4.5" for c in circles)  # >= 8 px markers
    titles = ["".join(t.itertext()) for t in root.iter("{http://www.w3.org/2000/svg}title")]
    assert "s0 <b>" in titles  # escaped in the markup, restored by the parser
    assert "task_a" in texts(root) and "0%" in texts(root) and "100%" in texts(root)


def seg_row(pcts, label="cat"):
    return {"label": label, "total_text": "n=1", "segments": pcts,
            "titles": {k: f"{k} {v}" for k, v in pcts.items()}}


def test_stacked_bars_labels_only_when_they_fit():
    root = parse(svg.stacked_bars([seg_row({"prefill": 60.0, "decode": 36.0, "tool": 0.4, "harness": 3.6})]))
    labels = texts(root)
    assert "60%" in labels and "36%" in labels
    assert "0%" not in [t for t in labels if t.endswith("%") and t not in ("0%", "25%", "50%", "75%", "100%")]
    assert "4%" not in labels  # 3.6 % of the plot is too narrow for its label + padding
    rects = list(root.iter("{http://www.w3.org/2000/svg}rect")) + list(root.iter("{http://www.w3.org/2000/svg}path"))
    # the 0.4 % tool segment is thinner than the 2 px gap and is not drawn (it stays in the table view)
    assert len([r for r in rects if "seg" in r.attrib.get("class", "")]) == 3


def test_stacked_bars_gap_thickness_and_rounded_end():
    root = parse(svg.stacked_bars([seg_row({"prefill": 50.0, "decode": 50.0})]))
    seg_rect = next(r for r in root.iter("{http://www.w3.org/2000/svg}rect") if "seg" in r.attrib.get("class", ""))
    assert float(seg_rect.attrib["height"]) <= 24  # thin marks
    assert abs(float(seg_rect.attrib["width"]) - (560 * 0.5 - 2)) < 0.01  # 2 px surface gap
    path = next(p for p in root.iter("{http://www.w3.org/2000/svg}path") if "seg" in p.attrib.get("class", ""))
    assert "a4.00,4.00" in path.attrib["d"]  # 4 px rounded data end on the last segment


def test_stacked_bars_segment_classes_follow_fixed_slots():
    root = parse(svg.stacked_bars([seg_row({"prefill": 25.0, "decode": 25.0, "tool": 25.0, "harness": 15.0,
                                            "unattributed": 10.0})]))
    classes = [e.attrib["class"].split()[-1] for e in root.iter() if "seg" in e.attrib.get("class", "").split()]
    assert classes == ["s1", "s2", "s3", "s4", "sx"]  # prefill, decode, tool, harness fixed; residual neutral


def pareto_pts():
    def p(label, x, y):
        return {"label": label, "x": x, "y": y, "x_lo": x * 0.9, "x_hi": x * 1.1, "y_lo": y - 0.1,
                "y_hi": min(1.0, y + 0.1), "title": label}
    return [p("a", 100, 0.5), p("b", 102, 0.52), p("c", 300, 0.9), p("d", 310, 0.3)]


def test_pareto_valid_svg_labels_and_front_classes():
    root = parse(svg.pareto(pareto_pts()))
    labels = texts(root)
    assert all(x in labels for x in ("a", "b", "c", "d")) and "50%" in labels
    groups = [g.attrib["class"] for g in root.iter("{http://www.w3.org/2000/svg}g")]
    assert groups.count("pt front") == 3 and groups.count("pt dominated") == 1
    assert any(p.attrib.get("class") == "frontier" for p in root.iter("{http://www.w3.org/2000/svg}path"))


def test_pareto_close_points_get_non_overlapping_labels():
    root = parse(svg.pareto(pareto_pts()))
    pos = {}
    for t in root.iter("{http://www.w3.org/2000/svg}text"):
        if t.attrib.get("class") == "pointlabel":
            pos[t.text] = (float(t.attrib["x"]), float(t.attrib["y"]))
    assert len(set(pos.values())) == 4  # a and b are 2 s apart, yet get different positions


def test_everything_user_derived_is_escaped():
    evil = '<script>alert(1)</script>'
    rows = [{"label": evil, "points": [{"series": 0, "v": 1, "lo": 0, "hi": 1, "title": evil}]}]
    out = svg.dot_whisker(rows, 1, evil, lambda v: str(v), 1.0, evil)
    assert "<script>" not in out
    parse(out)
    out2 = svg.stacked_bars([seg_row({"prefill": 100.0}, label=evil)])
    assert "<script>" not in out2
    parse(out2)
    pts = pareto_pts()
    pts[0]["label"] = evil
    pts[0]["title"] = evil
    assert "<script>" not in svg.pareto(pts)


def test_output_is_deterministic():
    assert svg.pareto(pareto_pts()) == svg.pareto(pareto_pts())
    r = [seg_row({"prefill": 33.3, "decode": 66.7})]
    assert svg.stacked_bars(r) == svg.stacked_bars(r)


@pytest.mark.parametrize("n_series", [1, 3, 8])
def test_dot_whisker_scales_with_series_count(n_series):
    rows = [{"label": "t", "points": [{"series": i, "v": 5, "lo": 4, "hi": 6, "title": "x"} for i in range(n_series)]}]
    root = parse(svg.dot_whisker(rows, n_series, "x", str))
    assert len(list(root.iter("{http://www.w3.org/2000/svg}circle"))) == n_series
