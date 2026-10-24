import json

from harness import specsweep as sw


def msg(name=None, args="{}", content="hi", reasoning=None):
    m = {"content": content, "reasoning_content": reasoning}
    if name:
        m["tool_calls"] = [{"function": {"name": name, "arguments": args}}]
    return m


def test_classify():
    assert sw.classify(msg()) == "prose"
    assert sw.classify(msg("edit")) == "tool_code"
    assert sw.classify(msg("write")) == "tool_code"
    assert sw.classify(msg("bash")) == "tool_other"


def test_text_sha_sensitive_to_every_part():
    a = sw.text_sha(msg("bash", '{"c":1}'))
    assert a == sw.text_sha(msg("bash", '{"c":1}'))
    assert a != sw.text_sha(msg("bash", '{"c":2}'))
    assert sw.text_sha(msg(content="x")) != sw.text_sha(msg(content="x", reasoning="think"))


def test_replay_body_is_greedy_and_capped():
    body = {"model": "m", "messages": [], "tools": [], "stream": True, "stream_options": {}, "seed": 9, "top_p": 1, "max_tokens": 32000}
    out = sw.replay_body(body, 1500)
    assert out["stream"] is False and out["temperature"] == 0 and out["max_tokens"] == 1500
    assert "stream_options" not in out and "top_p" not in out and out["seed"] == 1


def row(setting, call, kind, sha, n=100, ms=2000.0, drafted=0, acc=0):
    return {"setting": setting, "trial": "t", "call": call, "kind": kind, "sha": sha, "predicted_n": n, "predicted_ms": ms,
            "draft_n": drafted, "draft_accepted": acc}


def test_summarise_speed_acceptance_identity():
    rows = [row("none", 0, "prose", "a"), row("none", 1, "tool_code", "b"),
            row("mtp", 0, "prose", "a", ms=4000.0, drafted=40, acc=20), row("mtp", 1, "tool_code", "X", ms=1000.0, drafted=10, acc=10)]
    s = {(r["setting"], r["kind"]): r for r in sw.summarise(rows)}
    assert s["none", "all"]["tok_s"] == 50.0
    assert s["mtp", "prose"]["speed_vs_ref"] == 0.5 and s["mtp", "prose"]["acceptance"] == 0.5
    assert s["mtp", "tool_code"]["speed_vs_ref"] == 2.0
    assert (s["mtp", "prose"]["identical"], s["mtp", "tool_code"]["identical"]) == (1, 0)
    assert s["mtp", "all"]["identical"] == 1 and s["mtp", "all"]["compared"] == 2


def test_summarise_ignores_errors_and_render(tmp_path):
    rows = [row("none", 0, "prose", "a"), {"setting": "mtp", "trial": "t", "call": 0, "error": "boom"}]
    text = sw.render(sw.summarise(rows))
    assert "none" in text and "mtp" not in text


def test_load_trials(tmp_path):
    d = tmp_path / "trialA"
    d.mkdir()
    (d / "a1_002.json").write_text(json.dumps({"n": 2}))
    (d / "a1_001.json").write_text(json.dumps({"n": 1}))
    (tmp_path / "empty").mkdir()
    assert sw.load_trials(tmp_path) == {"trialA": [{"n": 1}, {"n": 2}]}
