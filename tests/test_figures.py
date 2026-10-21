import json
import xml.etree.ElementTree as ET
from pathlib import Path

from analysis import figures

SUMMARY = Path(__file__).parent.parent / "fixtures/replay/traced_pilot/expected/summary.json"


def test_figures_are_valid_standalone_svg(tmp_path):
    (tmp_path / "summary.json").write_text(SUMMARY.read_text())
    written = figures.write_figures(tmp_path)
    assert [p.name for p in written] == ["time_by_task_type.svg", "pass_rate_by_task.svg"]
    for p in written:
        root = ET.fromstring(p.read_text())
        assert root.tag.endswith("svg") and root.get("viewBox")
    assert "Prefill" in written[0].read_text()  # legend present


def test_figures_deterministic(tmp_path):
    (tmp_path / "summary.json").write_text(SUMMARY.read_text())
    a = [p.read_text() for p in figures.write_figures(tmp_path)]
    b = [p.read_text() for p in figures.write_figures(tmp_path)]
    assert a == b and json.loads(SUMMARY.read_text())["configs"]
