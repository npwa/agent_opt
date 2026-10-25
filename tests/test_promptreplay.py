import json
import time

import httpx
import pytest

from harness import promptreplay as pr

SSE = (b'data: {"choices":[{"delta":{"content":"x"}}]}\n\n'
       b'data: {"choices":[],"usage":{"prompt_tokens":100,"completion_tokens":1}}\n\n'
       b"data: [DONE]\n\n")

SYSTEM = ("BASE " * 50 + "<env>\n  Working directory: /tmp/agent_opt_scratch/cfg__task__00/work\n</env>\n"
          "Skills provide specialized instructions\n<available_skills><skill>a</skill></available_skills>")


def body(extra_msgs=()):
    return {"model": "m", "stream": True, "max_tokens": 32000, "tool_choice": "auto",
            "messages": [{"role": "system", "content": SYSTEM}, {"role": "user", "content": "go"}, *extra_msgs],
            "tools": [{"type": "function", "function": {"name": n, "description": n * 20}} for n in ("bash", "read", "task", "skill")]}


class Slow(httpx.SyncByteStream):
    def __init__(self, delay):
        self.delay = delay

    def __iter__(self):
        time.sleep(self.delay)  # time to first token
        yield SSE


class FakeServer:
    """Fake Ollama: ttft grows with the prompt size unless the prompt starts like the previous one (cache)."""
    def __init__(self):
        self.calls = []  # (path, body)
        self.last = ""

    def handler(self, request: httpx.Request) -> httpx.Response:
        b = json.loads(request.content)
        self.calls.append((request.url.path, b))
        if request.url.path.endswith("/api/generate"):
            return httpx.Response(200, json={})
        text = json.dumps(b.get("messages")) + json.dumps(b.get("tools"))
        shared = len(__import__("os").path.commonprefix([self.last, text]))
        self.last = text
        return httpx.Response(200, stream=Slow(0.00002 * (len(text) - shared)), headers={"content-type": "text/event-stream"})


@pytest.fixture
def server():
    s = FakeServer()
    client = httpx.Client(transport=httpx.MockTransport(s.handler), timeout=None)
    return s, client


def test_variants_transform_bodies_without_mutating_the_input():
    b = body()
    names = lambda x: [t["function"]["name"] for t in x["tools"]]  # noqa: E731
    assert names(pr.make_variant("drop_tools=task+skill")(b)) == ["bash", "read"]
    assert names(pr.make_variant("keep_tools=bash")(b)) == ["bash"]
    assert "Skills provide" not in pr.make_variant("strip_skills")(b)["messages"][0]["content"]
    assert pr.make_variant("default")(b) is b
    assert names(b) == ["bash", "read", "task", "skill"] and "Skills provide" in b["messages"][0]["content"]
    assert "tools" not in pr.drop_tools(b, {"bash", "read", "task", "skill"})
    for bad in ("nope", "drop_tools=", "keep_tools"):
        with pytest.raises(ValueError):
            pr.make_variant(bad)


def test_perturb_changes_only_the_scratch_directory():
    b = body([{"role": "tool", "content": "cat /tmp/agent_opt_scratch/cfg__task__00/work/a.c"}])
    p = pr.perturb(b, "replay_1")
    s = json.dumps(p)
    assert "cfg__task__00" not in s and s.count("/tmp/agent_opt_scratch/replay_1/") == 2
    assert p["tools"] == b["tools"] and p["messages"][1] == b["messages"][1]


def test_perturb_keeps_the_shared_tmp_directory():
    """Every trial has the same shared TMPDIR (it sits in the bash tool text); changing it would defeat the prefix cache."""
    b = body([{"role": "tool", "content": "use /tmp/agent_opt_scratch/shared_tmp/opencode in /tmp/agent_opt_scratch/cfg__t__00/work"}])
    s = json.dumps(pr.perturb(b, "replay_1"))
    assert "/tmp/agent_opt_scratch/shared_tmp/opencode" in s and "cfg__t__00" not in s


def test_add_nonce_marks_system_and_first_tool_only():
    b = body()
    n = pr.add_nonce(b, "abc")
    assert n["messages"][0]["content"].startswith("[replay abc] BASE") and n["tools"][0]["function"]["description"].startswith("[replay abc] ")
    assert n["messages"][1] == b["messages"][1] and n["tools"][1] == b["tools"][1]
    assert "[replay" not in json.dumps(b)  # input untouched
    nosys = {"messages": [{"role": "user", "content": "q"}]}
    assert pr.add_nonce(nosys, "z")["messages"][0]["content"].startswith("[replay z] q")


def test_load_sequence_orders_calls(tmp_path):
    for i in (2, 10, 1):
        (tmp_path / f"a1_{i:03d}.json").write_text(json.dumps({"n": i}))
    assert [b["n"] for b in pr.load_sequence(tmp_path)] == [1, 2, 10]
    assert pr.load_sequence(tmp_path / "a1_002.json") == [{"n": 2}]
    with pytest.raises(FileNotFoundError):
        pr.load_sequence(tmp_path / "empty")


def test_post_timed_measures_first_chunk_and_usage(server):
    s, client = server
    r = pr.post_timed(client, "http://x/v1/chat/completions", pr.prepare(body(), 1, None))
    assert r["status"] == 200 and r["prompt_tokens"] == 100 and r["completion_tokens"] == 1 and r["ttft_s"] > 0
    sent = s.calls[0][1]
    assert sent["max_tokens"] == 1 and sent["stream"] is True and sent["stream_options"] == {"include_usage": True}


def test_flush_modes(server):
    s, client = server
    pr.flush_cache(client, "http://x/v1", "m", "none")
    assert not s.calls
    pr.flush_cache(client, "http://x/v1", "m", "dummy")
    assert [p for p, _ in s.calls] == ["/v1/chat/completions"]
    s.calls.clear()
    pr.flush_cache(client, "http://x/v1", "m", "unload")
    assert [p for p, _ in s.calls] == ["/api/generate", "/v1/chat/completions", "/v1/chat/completions"]
    assert s.calls[0][1] == {"model": "m", "keep_alive": 0}


def test_experiment_shows_cold_slower_than_warm_and_trimming_faster(server):
    s, client = server
    seq = [body(), body([{"role": "assistant", "content": "ok"}, {"role": "user", "content": "more"}])]
    rows = pr.experiment(client, "http://x/v1", seq, ["default", "drop_tools=task+skill", "strip_skills"], repeats=3, seed=1)
    assert len(rows) == 3 * 3 * 2
    summ = {(r["variant"], r["mode"]): r for r in pr.summarise(rows)}
    for v in ("default", "drop_tools=task+skill", "strip_skills"):
        assert summ[(v, "cold")]["first_call_ttft_s"] > summ[(v, "warm")]["first_call_ttft_s"]
    assert summ[("drop_tools=task+skill", "cold")]["first_call_ttft_s"] < summ[("default", "cold")]["first_call_ttft_s"]
    assert summ[("strip_skills", "cold")]["first_call_ttft_s"] < summ[("default", "cold")]["first_call_ttft_s"]
    assert "vs_default_sequence_ratio" in summ[("strip_skills", "cold")] and "vs_default_sequence_ratio" not in summ[("default", "cold")]
    md = pr.render(list(summ.values()), {"sequence": "x", "calls": 2, "repeats": 3, "flush": "dummy", "max_tokens": 1, "model": "m"})
    assert "| strip_skills | cold |" in md and "directional" in md  # n = 3 < 5


def test_nonce_flush_makes_every_cold_run_cold_even_when_the_cache_holds_the_prefix(server):
    s, client = server
    seq = [body()]
    # prime the fake server's cache with the unmarked prompt, then run cold: must not benefit from it
    pr.run_sequence(client, "http://x/v1", seq, 1, None)
    warm = pr.run_sequence(client, "http://x/v1", seq, 1, None)[0]["ttft_s"]
    rows = pr.experiment(client, "http://x/v1", seq, ["default"], repeats=2, flush="nonce")
    cold = [r["calls"][0]["ttft_s"] for r in rows if r["mode"] == "cold"]
    assert all(c > 5 * warm for c in cold)


def test_faithful_mode_keeps_recorded_max_tokens(server):
    s, client = server
    pr.run_sequence(client, "http://x/v1", [body()], None, None)
    assert s.calls[0][1]["max_tokens"] == 32000
