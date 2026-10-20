import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from analysis import report, stats
from harness import decompose
from tests.synth import CONFIGS, TASKS, make_run

ROOT = Path(__file__).resolve().parent.parent
TASKS_DIR = ROOT / "tasks"


@pytest.fixture(scope="module")
def run(tmp_path_factory):
    return make_run(tmp_path_factory.mktemp("synth") / "synthetic_run")


@pytest.fixture(scope="module")
def summary(run):
    return report.summarize(report.load_run(run, TASKS_DIR))


def test_counts_and_overview_match_raw_records(run, summary):
    recs = [json.loads(p.read_text()) for p in (run / "trials").glob("*/record.json")]
    assert summary["trials"]["measured"] == len(recs) == 3 * 4 * 8
    for c in CONFIGS:
        mine = [r for r in recs if r["config"] == c]
        o = summary["overview"][c]
        assert o["n"] == len(mine) and o["passes"] == sum(r["status"] == "pass" for r in mine)
        assert o["wilson"] == pytest.approx(list(stats.wilson_interval(o["passes"], o["n"])))
    assert summary["baseline"] == "baseline_32k" and summary["common_tasks"] == sorted(TASKS)


def test_cell_statistics(summary):
    cell = summary["cells"]["baseline_32k|bugfix_01_moving_average"]
    assert cell["n"] == 8 and cell["median_ci"][0] <= cell["median_s"] <= cell["median_ci"][1]
    assert cell["wilson"][0] <= cell["rate"] <= cell["wilson"][1]


def test_decomposition_buckets_sum_to_100_and_unattributed_is_zero(summary):
    for d in summary["decomposition"].values():
        assert sum(d["all"]["pct"].values()) == pytest.approx(100.0)
        assert abs(d["all"]["pct"]["unattributed"]) < 1e-6 and d["flagged"] == []
        assert set(d["categories"]) == {"code_gen", "reasoning", "bugfix", "repo_qa"}


def test_comparisons_detect_faster_and_slower_configs(summary):
    by = {(r["config"], r["category"]): r for r in summary["comparisons"]}
    assert len(by) == 8  # 2 non-baseline configs x 4 categories
    fast = by[("trim_schemas", "code_gen")]
    assert fast["time"]["ratio_median"] < 0.7 and fast["time"]["improved"] and fast["p_holm"] < 0.05
    slow = by[("ctx_8k", "bugfix")]
    assert slow["time"]["ratio_median"] > 1.0 and slow["verdict"] == "negative"
    assert all(r["p_holm"] >= r["time"]["p"] - 1e-12 for r in summary["comparisons"])  # Holm never lowers p


def test_verdict_rule_requires_no_success_drop(run):
    data = report.load_run(run, TASKS_DIR)
    for r in data["records"]:  # make trim_schemas fail much more often: faster but worse
        if r["config"] == "trim_schemas":
            r["status"] = "fail"
    s = report.summarize(data)
    for row in s["comparisons"]:
        if row["config"] == "trim_schemas":
            assert row["time"]["improved"] and row["verdict"] != "positive"
            assert row["verdict"] == "negative"  # an established > 5 pp success drop


def test_baseline_choice_and_unknown_baseline(run):
    data = report.load_run(run, TASKS_DIR)
    s = report.summarize(data, baseline="trim_schemas")
    assert s["baseline"] == "trim_schemas" and {r["config"] for r in s["comparisons"]} == {"baseline_32k", "ctx_8k"}
    with pytest.raises(ValueError, match="not in run configs"):
        report.summarize(data, baseline="nope")


def test_harness_failures_and_parity_trials_are_excluded(run, tmp_path):
    import shutil
    d = tmp_path / "r"
    shutil.copytree(run, d)
    tid = "baseline_32k__coding_02_fizzbuzz_gen__00"
    rec = json.loads((d / "trials" / tid / "record.json").read_text())
    rec["status"] = "harness_failed"
    (d / "trials" / tid / "record.json").write_text(json.dumps(rec))
    par = d / "trials" / "baseline_32k__coding_02_fizzbuzz_gen__p00"
    par.mkdir()
    (par / "record.json").write_text(json.dumps({**rec, "trial_id": par.name, "status": "pass", "instrumented": False}))
    s = report.summarize(report.load_run(d, TASKS_DIR))
    assert s["trials"]["harness_failed"] == 1 and s["parity_trials"] == 1
    assert s["overview"]["baseline_32k"]["n"] == 31 and s["cells"]["baseline_32k|coding_02_fizzbuzz_gen"]["n"] == 7


def test_directional_marking_for_small_n(tmp_path):
    run = make_run(tmp_path / "small", n=3)
    s = report.summarize(report.load_run(run, TASKS_DIR))
    assert all(r["time"]["directional"] for r in s["comparisons"])
    md = report.render_markdown(s)
    assert "directional" in md and " 3* " in md


def test_run_without_traces_still_reports(tmp_path):
    run = make_run(tmp_path / "notrace", with_traces=False)
    s = report.write_report(run, tasks_dir=TASKS_DIR)
    assert s["decomposition"] == {}
    assert "No traces in this run" in (run / "RESULTS.md").read_text()
    assert "No traces in this run" in (run / "report.html").read_text()


def test_single_config_has_no_comparisons(tmp_path):
    run = make_run(tmp_path / "one", configs=["baseline_32k"])
    s = report.write_report(run, tasks_dir=TASKS_DIR)
    assert s["comparisons"] == []
    assert "nothing to compare" in (run / "RESULTS.md").read_text()


def test_markdown_content_and_no_negative_zero(run):
    md = report.render_markdown(report.summarize(report.load_run(run, TASKS_DIR)))
    for heading in ("## Configurations", "## Per task", "## Where the time goes", "## Comparison with the baseline"):
        assert heading in md
    assert "-0.0" not in md and "report.html" in md and "baseline_32k" in md


def test_html_is_self_contained_with_table_twins_and_theme_support(run):
    html = report.render_html(report.summarize(report.load_run(run, TASKS_DIR)))
    assert "<script" not in html and "http://" not in html.replace("http://www.w3.org/2000/svg", "")
    assert "https://" not in html
    assert html.count("<svg") == 1 + 2 + 3  # pareto + two per-task charts + three decomposition charts
    assert html.count("Table view") == 2 + 3  # every chart except the Pareto has a table twin (the config table)
    for needle in ('prefers-color-scheme:dark', 'data-theme="dark"', "forced-colors", "--series-1",
                   'role="img"', "<title>agent_opt results"):
        assert needle in html
    for svg_src in re.findall(r"<svg.*?</svg>", html, flags=re.S):
        ET.fromstring(svg_src)


def test_untrusted_names_are_escaped_everywhere(run, tmp_path):
    import shutil
    d = tmp_path / "evil"
    shutil.copytree(run, d)
    plan = json.loads((d / "plan.json").read_text())
    plan["configs"]["<img src=x onerror=alert(1)>"] = {"id": "x"}
    (d / "plan.json").write_text(json.dumps(plan))
    for p in (d / "trials").glob("baseline_32k__*"):
        rec = json.loads((p / "record.json").read_text())
        evil = p.parent / p.name.replace("baseline_32k", "evil")
        evil.mkdir()
        (evil / "record.json").write_text(json.dumps({**rec, "config": "<img src=x onerror=alert(1)>",
                                                      "trial_id": evil.name}))
    s = report.write_report(d, tasks_dir=TASKS_DIR)
    html = (d / "report.html").read_text()
    assert "<img src=x" not in html and "&lt;img src=x" in html and s["configs"][-1].startswith("<img")


def test_report_is_deterministic(run, tmp_path):
    a = report.write_report(run, tmp_path / "a", TASKS_DIR)
    b = report.write_report(run, tmp_path / "b", TASKS_DIR)
    assert a == b
    for name in ("RESULTS.md", "report.html", "summary.json"):
        assert (tmp_path / "a" / name).read_bytes() == (tmp_path / "b" / name).read_bytes()


def test_decomposition_agrees_with_harness_module(run, summary):
    recs = [r for r in report.load_run(run, TASKS_DIR)["records"] if r["config"] == "baseline_32k"]
    ds = [decompose.decompose(r["_trace"]) for r in recs]
    agg = decompose.aggregate(ds, report.load_run(run, TASKS_DIR)["categories"])
    assert summary["decomposition"]["baseline_32k"]["categories"]["bugfix"]["pct"] == pytest.approx(agg["bugfix"]["pct"])


def test_cli_writes_outputs(run, tmp_path, monkeypatch, capsys):
    out = tmp_path / "out"
    monkeypatch.setattr("sys.argv", ["report", "--run", str(run), "--out", str(out), "--tasks", str(TASKS_DIR)])
    assert report.main() == 0
    assert {p.name for p in out.iterdir()} == {"RESULTS.md", "report.html", "summary.json"}
    assert "96 measured trials" in capsys.readouterr().out
