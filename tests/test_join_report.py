import csv

from harness import join_metrics, report


def test_join_windows_and_missing_gpu(fixtures_dir):
    rows = join_metrics.join(
        join_metrics.load_records(fixtures_dir / "metrics.jsonl"),
        join_metrics.load_samples(fixtures_dir / "samples.csv"),
    )
    assert [r["avg_gpu_util_pct"] for r in rows] == [50.0, 20.0, None]
    assert [r["avg_cpu_pct"] for r in rows] == [25.0, 15.0, 5.0]
    assert rows[0]["avg_ram_pct"] == 50.0


def test_report_end_to_end(tmp_path, monkeypatch, fixtures_dir, capsys):
    rows = join_metrics.join(
        join_metrics.load_records(fixtures_dir / "metrics.jsonl"),
        join_metrics.load_samples(fixtures_dir / "samples.csv"),
    )
    join_metrics.write_csv(rows, tmp_path / "benchmark_results.csv")
    monkeypatch.setattr(report, "IN_PATH", tmp_path / "benchmark_results.csv")
    monkeypatch.setattr(report, "OUT_PATH", tmp_path / "report.md")
    report.main()
    text = (tmp_path / "report.md").read_text()
    assert "## t_a" in text and "## t_b" in text
    assert "- Calls: 2" in text
    assert "- Avg time to first token: 3.0 s" in text
    assert "- Avg completion tokens/call: 60.0 (approx.)" in text
    assert "- Avg GPU utilization during call: 35.0%" in text
    assert "- Avg input throughput: None tok/s" in text  # t_b has no prompt tokens


def test_report_missing_input(tmp_path, monkeypatch, capsys):
    monkeypatch.setattr(report, "IN_PATH", tmp_path / "nope.csv")
    report.main()
    assert "run join_metrics.py first" in capsys.readouterr().out


def test_csv_header_matches_report_contract(tmp_path):
    join_metrics.write_csv([], tmp_path / "x.csv")
    header = next(csv.reader(open(tmp_path / "x.csv")))
    for col in ("task", "ttft_s", "total_s", "input_tok_s", "output_tok_s",
                "avg_gpu_util_pct", "avg_cpu_pct", "avg_ram_pct", "completion_tokens"):
        assert col in header
