import json

import pytest

from analysis import consistency


def make(tmp_path, rows):
    for i, (cfg, task, status) in enumerate(rows):
        d = tmp_path / "trials" / f"{cfg}__{task}__{i:02d}"
        d.mkdir(parents=True)
        (d / "record.json").write_text(json.dumps({"config": cfg, "task": task, "status": status, "instrumented": True}))
    return tmp_path


def test_cells_are_classified_and_variance_is_within_cell(tmp_path):
    rows = [("a", "t1", "pass")] * 3 + [("a", "t2", "fail")] * 3 + [("a", "t3", "pass"), ("a", "t3", "fail"), ("a", "t3", "fail")]
    rows += [("b", "t1", "pass"), ("b", "t1", "fail")] + [("b", "t2", "harness_failed")] * 2
    res = consistency.analyse(make(tmp_path, rows))
    assert res["a"]["all_pass"] == 1 and res["a"]["all_fail"] == 1 and res["a"]["mixed"] == 1
    assert res["a"]["mean_within_cell_variance"] == pytest.approx((0 + 0 + (1 / 3) * (2 / 3)) / 3)
    assert res["b"]["cells"] == 1 and res["b"]["mean_within_cell_variance"] == pytest.approx(0.25)  # harness failures are not trials
    assert "| a | 3 | 9 | 44 % |" in consistency.render("x", res)
