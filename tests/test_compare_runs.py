from pathlib import Path

import pytest

from analysis import compare_runs

ROOT = Path(__file__).parent.parent / "results/v1"
A, B = ROOT / "baseline", ROOT / "baseline_isolated"


@pytest.mark.skipif(not (A / "summary.json").exists() or not (B / "summary.json").exists(), reason="recorded runs absent")
def test_compare_recorded_runs():
    text = compare_runs.compare(A, B)
    assert "## Success" in text and "## Time per trial" in text and "Order dependence" in text
    assert "A 104/140" in text and "B 99/140" in text  # recorded counts
    assert "Pooled, normalised" in text


def test_compare_with_itself_is_neutral():
    if not (A / "summary.json").exists():
        pytest.skip("recorded run absent")
    text = compare_runs.compare(A, A)
    assert "difference +0.0 pp" in text and "median ratio B/A = 1.00" in text
