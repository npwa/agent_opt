"""Characterization tests for harness.proxy against a mocked Ollama upstream."""

import json

import httpx
import pytest

from harness import proxy


def make_upstream(fixtures_dir, captured):
    sse = (fixtures_dir / "sse_usage_stream.txt").read_bytes()

    def handler(request: httpx.Request) -> httpx.Response:
        captured.append(request)
        if request.url.path.endswith("chat/completions"):
            return httpx.Response(200, content=sse,
                                  headers={"content-type": "text/event-stream"})
        return httpx.Response(200, json={"models": []})

    return httpx.AsyncClient(transport=httpx.MockTransport(handler), timeout=None)


@pytest.fixture
def wired(tmp_path, monkeypatch, fixtures_dir):
    captured: list[httpx.Request] = []
    monkeypatch.setattr(proxy, "client", make_upstream(fixtures_dir, captured))
    monkeypatch.setattr(proxy, "DATA_DIR", tmp_path)
    monkeypatch.setattr(proxy, "LOG_PATH", tmp_path / "metrics.jsonl")
    monkeypatch.setattr(proxy, "TASK_LABEL_PATH", tmp_path / "current_task.txt")
    transport = httpx.ASGITransport(app=proxy.app)
    return httpx.AsyncClient(transport=transport, base_url="http://proxy"), captured, tmp_path


async def test_chat_forwards_stream_and_logs_record(wired, fixtures_dir):
    c, captured, tmp = wired
    (tmp / "current_task.txt").write_text("coding_01\n")
    r = await c.post("/v1/chat/completions", json={"model": "m", "messages": []})
    assert r.status_code == 200
    # client receives the upstream stream including blank-line event delimiters
    assert r.content == (fixtures_dir / "sse_usage_stream.txt").read_bytes()

    rec = json.loads((tmp / "metrics.jsonl").read_text().strip())
    assert rec["task"] == "coding_01"
    assert rec["prompt_tokens"] == 120
    assert rec["completion_tokens"] == 2
    assert rec["completion_tokens_is_approx"] is False
    assert rec["total_s"] >= rec["ttft_s"] >= 0


async def test_chat_forces_stream_true_and_does_not_add_stream_options(wired):
    c, captured, _ = wired
    await c.post("/v1/chat/completions", json={"model": "m", "stream": False})
    body = json.loads(captured[0].content)
    assert body["stream"] is True
    assert "stream_options" not in body  # v0 failure mode: changes reasoning/content split


async def test_content_length_matches_modified_body(wired):
    c, captured, _ = wired
    await c.post("/v1/chat/completions", json={"model": "m"})
    req = captured[0]
    assert int(req.headers["content-length"]) == len(req.content)


async def test_non_chat_passthrough_is_not_logged(wired):
    c, captured, tmp = wired
    r = await c.get("/v1/models")
    assert r.json() == {"models": []}
    assert not (tmp / "metrics.jsonl").exists()


async def test_unlabeled_when_no_task_file(wired):
    c, _, tmp = wired
    await c.post("/v1/chat/completions", json={"model": "m"})
    assert json.loads((tmp / "metrics.jsonl").read_text())["task"] == "unlabeled"


async def test_approx_tokens_when_no_usage(tmp_path, monkeypatch):
    sse = b'data: {"choices":[{"delta":{"content":"a b c"}}]}\n\ndata: [DONE]\n\n'
    up = httpx.AsyncClient(
        transport=httpx.MockTransport(lambda r: httpx.Response(200, content=sse)), timeout=None)
    monkeypatch.setattr(proxy, "client", up)
    monkeypatch.setattr(proxy, "DATA_DIR", tmp_path)
    monkeypatch.setattr(proxy, "LOG_PATH", tmp_path / "metrics.jsonl")
    monkeypatch.setattr(proxy, "TASK_LABEL_PATH", tmp_path / "current_task.txt")
    c = httpx.AsyncClient(transport=httpx.ASGITransport(app=proxy.app), base_url="http://p")
    await c.post("/v1/chat/completions", json={"model": "m"})
    rec = json.loads((tmp_path / "metrics.jsonl").read_text())
    assert rec["completion_tokens_is_approx"] is True
    assert rec["completion_tokens"] == 3
