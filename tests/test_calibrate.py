import json

from harness import calibrate
from harness.taskspec import load_task
from tests.test_verifiers import ROOT


def test_parse_events_extracts_tools_tokens_and_final_text(tmp_path):
    ev = [
        {"type": "text", "sessionID": "s1", "part": {"text": "thinking out loud"}},
        {"type": "tool_use", "sessionID": "s1", "part": {"tool": "read", "state": {"status": "completed"}}},
        {"type": "tool_use", "sessionID": "s1", "part": {"tool": "bash", "state": {"status": "error"}}},
        {"type": "step_finish", "sessionID": "s1", "part": {"tokens": {"input": 10, "output": 3}}},
        {"type": "text", "sessionID": "s1", "part": {"text": "final answer"}},
        {"type": "step_finish", "sessionID": "s1", "part": {"tokens": {"input": 20, "output": 4}}},
    ]
    p = tmp_path / "e.jsonl"
    p.write_text("\n".join(json.dumps(e) for e in ev) + "\nnot json\n")
    r = calibrate.parse_events(p)
    assert r["tools"] == ["read", "bash"] and r["tool_errors"] == 1
    assert (r["steps"], r["tokens_in"], r["tokens_out"]) == (2, 30, 7)
    assert r["final_text"] == "final answer" and r["session"] == "s1"


def test_parse_events_missing_file(tmp_path):
    assert calibrate.parse_events(tmp_path / "none")["tools"] == []


def test_subsequence_matching_allows_mcp_prefix():
    assert calibrate.is_subsequence(["read", "send_email"], ["glob", "read", "comm-tools_send_email"])
    assert not calibrate.is_subsequence(["write", "read"], ["read", "write"])
    assert calibrate.is_subsequence([], [])


def test_build_config_isolation_and_tools():
    spec = load_task(ROOT / "tasks/research_08e_free_will.yaml")
    cfg = calibrate.build_config(spec, "ollama/qwen3.6-32k", None)
    assert cfg["provider"]["ollama"]["options"]["baseURL"].endswith(":11435/v1")
    assert "searxng" in cfg["mcp"] and "comm-tools" not in cfg["mcp"]
    assert cfg["tools"]["webfetch"] is True and cfg["tools"]["bash"] is False
    spec6 = load_task(ROOT / "tasks/coding_06_tax_rates_lan.yaml")
    assert not any(calibrate.build_config(spec6, "ollama/m", None)["tools"].values())


def test_changed_files_reports_new_and_modified(tmp_path):
    fx, work = tmp_path / "fx", tmp_path / "work"
    fx.mkdir()
    work.mkdir()
    (fx / "a.txt").write_text("1")
    (work / "a.txt").write_text("1")
    (work / "b.txt").write_text("new")
    assert list(calibrate.changed_files(fx, work)) == ["b.txt"]
    (work / "a.txt").write_text("2")
    assert sorted(calibrate.changed_files(fx, work)) == ["a.txt", "b.txt"]


def test_changed_files_skips_binaries(tmp_path):
    work = tmp_path / "work"
    work.mkdir()
    (work / "prog").write_bytes(b"\x7fELF\x00\x01")
    (work / "a.c").write_text("int x;")
    assert list(calibrate.changed_files(None, work)) == ["a.c"]


def test_build_config_trial_headers():
    spec = load_task(ROOT / "tasks/coding_05_scientific_lan.yaml")
    opts = calibrate.build_config(spec, "ollama/m", None, "t-1")["provider"]["ollama"]["options"]
    assert opts["headers"] == {"X-Trial-Id": "t-1", "X-Task-Id": spec.id}
    assert "headers" not in calibrate.build_config(spec, "ollama/m", None)["provider"]["ollama"]["options"]
