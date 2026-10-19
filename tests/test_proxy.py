"""Tests for harness.proxy against a mocked Ollama upstream.

Legacy mode (v0 behaviour) is characterised first; passthrough mode (default) is tested
together with the three fault-injection cases from the plan (7.2): a modified body with a
stale Content-Length, an SSE stream without its blank-line delimiter, and an injected
``stream_options``. In passthrough mode the proxy must neither alter the request nor the
response bytes.
"""

import json

import httpx
import pytest

from harness import proxy

SSE = (b'data: {"choices":[{"delta":{"content":"Hello"}}]}\n\n'
       b'data: {"choices":[{"delta":{"content":" world"}}]}\n\n'
       b'data: {"choices":[],"usage":{"prompt_tokens":120,"completion_tokens":2}}\n\n'
       b"data: [DONE]\n\n")


class Chunks(httpx.AsyncByteStream):
    def __init__(self, chunks):
        self.chunks = chunks

    async def __aiter__(self):
        for c in self.chunks:
            yield c


def split(data: bytes, size: int) -> list[bytes]:
    return [data[i:i + size] for i in range(0, len(data), size)]


def make_wired(monkeypatch, tmp_path, chunks, *, passthrough, captured, status=200):
    def handler(request: httpx.Request) -> httpx.Response:
        captured.append(request)
        if request.url.path.endswith("chat/completions"):
            return httpx.Response(status, stream=Chunks(chunks),
                                  headers={"content-type": "text/event-stream"})
        return httpx.Response(200, stream=Chunks([b'{"models": []}']),
                              headers={"content-type": "application/json"})

    monkeypatch.setattr(proxy, "client",
                        httpx.AsyncClient(transport=httpx.MockTransport(handler), timeout=None))
    monkeypatch.setattr(proxy, "PASSTHROUGH", passthrough)
    monkeypatch.setattr(proxy, "DATA_DIR", tmp_path)
    monkeypatch.setattr(proxy, "LOG_PATH", tmp_path / "metrics.jsonl")
    monkeypatch.setattr(proxy, "TASK_LABEL_PATH", tmp_path / "current_task.txt")
    return httpx.AsyncClient(transport=httpx.ASGITransport(app=proxy.app), base_url="http://proxy")


def last_record(tmp_path):
    return json.loads((tmp_path / "metrics.jsonl").read_text().strip().splitlines()[-1])


@pytest.fixture
def legacy(monkeypatch, tmp_path):
    captured: list[httpx.Request] = []
    return make_wired(monkeypatch, tmp_path, [SSE], passthrough=False, captured=captured), \
        captured, tmp_path


# ---- legacy mode (v0 characterisation) -------------------------------------------------

async def test_legacy_logs_record_and_forwards_stream(legacy):
    c, _, tmp = legacy
    (tmp / "current_task.txt").write_text("coding_01\n")
    r = await c.post("/v1/chat/completions", json={"model": "m", "messages": []})
    assert r.content == SSE
    rec = last_record(tmp)
    assert (rec["task"], rec["prompt_tokens"], rec["completion_tokens"]) == ("coding_01", 120, 2)
    assert rec["completion_tokens_is_approx"] is False


async def test_legacy_forces_stream_true_without_stream_options(legacy):
    c, captured, _ = legacy
    await c.post("/v1/chat/completions", json={"model": "m", "stream": False})
    body = json.loads(captured[0].content)
    assert body["stream"] is True and "stream_options" not in body
    assert int(captured[0].headers["content-length"]) == len(captured[0].content)


# ---- passthrough mode (default) ----------------------------------------------------------

def passthrough_client(monkeypatch, tmp_path, chunks, status=200):
    captured: list[httpx.Request] = []
    c = make_wired(monkeypatch, tmp_path, chunks, passthrough=True, captured=captured, status=status)
    return c, captured


async def test_passthrough_is_default():
    assert proxy.PASSTHROUGH is True


async def test_passthrough_request_bytes_identical_even_with_odd_body(monkeypatch, tmp_path):
    """Fault: a proxy that rewrites the body breaks Content-Length (v0 failure 1)."""
    c, captured = passthrough_client(monkeypatch, tmp_path, [SSE])
    body = b'{ "model" : "m",   "stream":false,  "messages":[] }'  # odd spacing, stream:false
    await c.post("/v1/chat/completions", content=body,
                 headers={"content-type": "application/json"})
    up = captured[0]
    assert up.content == body
    assert int(up.headers["content-length"]) == len(body)


async def test_passthrough_does_not_inject_stream_options(monkeypatch, tmp_path):
    """Fault: injected stream_options changed reasoning/content splitting (v0 failure 3)."""
    c, captured = passthrough_client(monkeypatch, tmp_path, [SSE])
    payload = json.dumps({"model": "m", "messages": [], "stream": True}).encode()
    await c.post("/v1/chat/completions", content=payload)
    assert captured[0].content == payload
    assert b"stream_options" not in captured[0].content


async def test_passthrough_stream_bytes_identical_across_chunk_boundaries(monkeypatch, tmp_path):
    """Chunks split mid-line must reach the client unchanged."""
    c, _ = passthrough_client(monkeypatch, tmp_path, split(SSE, 7))
    r = await c.post("/v1/chat/completions", json={"model": "m"})
    assert r.content == SSE
    rec = last_record(tmp_path)
    assert rec["prompt_tokens"] == 120 and rec["completion_tokens"] == 2
    assert rec["chunks"] == len(split(SSE, 7)) and rec["aborted"] is False


async def test_passthrough_missing_blank_line_delimiter_not_repaired(monkeypatch, tmp_path):
    """Fault: SSE without the blank-line delimiter (v0 failure 2) - bytes pass through as sent."""
    broken = b'data: {"choices":[{"delta":{"content":"a"}}]}\ndata: [DONE]\n'
    c, _ = passthrough_client(monkeypatch, tmp_path, [broken])
    r = await c.post("/v1/chat/completions", json={"model": "m"})
    assert r.content == broken


async def test_passthrough_crlf_stream_unchanged(monkeypatch, tmp_path):
    crlf = SSE.replace(b"\n", b"\r\n")
    c, _ = passthrough_client(monkeypatch, tmp_path, split(crlf, 11))
    r = await c.post("/v1/chat/completions", json={"model": "m"})
    assert r.content == crlf
    assert last_record(tmp_path)["prompt_tokens"] == 120


async def test_passthrough_trial_headers_logged_and_stripped(monkeypatch, tmp_path):
    c, captured = passthrough_client(monkeypatch, tmp_path, [SSE])
    await c.post("/v1/chat/completions", json={"model": "m"},
                 headers={"X-Trial-Id": "t-42", "X-Task-Id": "bugfix_01"})
    rec = last_record(tmp_path)
    assert (rec["trial_id"], rec["task"]) == ("t-42", "bugfix_01")
    assert "x-trial-id" not in captured[0].headers and "x-task-id" not in captured[0].headers


async def test_passthrough_label_file_fallback_and_no_trial(monkeypatch, tmp_path):
    (tmp_path / "current_task.txt").write_text("from_file")
    c, _ = passthrough_client(monkeypatch, tmp_path, [SSE])
    await c.post("/v1/chat/completions", json={"model": "m"})
    rec = last_record(tmp_path)
    assert rec["task"] == "from_file" and rec["trial_id"] is None


async def test_passthrough_upstream_error_status_forwarded_and_logged(monkeypatch, tmp_path):
    c, _ = passthrough_client(monkeypatch, tmp_path, [b'{"error":"boom"}'], status=500)
    r = await c.post("/v1/chat/completions", json={"model": "m"})
    assert r.status_code == 500 and r.content == b'{"error":"boom"}'
    assert last_record(tmp_path)["status"] == 500


async def test_passthrough_non_streaming_json_usage(monkeypatch, tmp_path):
    body = b'{"choices":[{"message":{"content":"x"}}],"usage":{"prompt_tokens":9,"completion_tokens":4}}'
    c, _ = passthrough_client(monkeypatch, tmp_path, [body])
    r = await c.post("/v1/chat/completions", json={"model": "m", "stream": False})
    assert r.content == body
    rec = last_record(tmp_path)
    assert (rec["prompt_tokens"], rec["completion_tokens"]) == (9, 4)


async def test_passthrough_approx_tokens_without_usage(monkeypatch, tmp_path):
    sse = b'data: {"choices":[{"delta":{"content":"a b c"}}]}\n\ndata: [DONE]\n\n'
    c, _ = passthrough_client(monkeypatch, tmp_path, [sse])
    await c.post("/v1/chat/completions", json={"model": "m"})
    rec = last_record(tmp_path)
    assert rec["completion_tokens_is_approx"] is True and rec["completion_tokens"] == 3


async def test_passthrough_non_chat_forwarded_not_logged(monkeypatch, tmp_path):
    c, _ = passthrough_client(monkeypatch, tmp_path, [SSE])
    r = await c.get("/v1/models")
    assert r.json() == {"models": []}
    assert not (tmp_path / "metrics.jsonl").exists()


async def test_passthrough_writes_no_debug_dumps(monkeypatch, tmp_path):
    c, _ = passthrough_client(monkeypatch, tmp_path, [SSE])
    await c.post("/v1/chat/completions", json={"model": "m"})
    assert not (tmp_path / "last_request_raw.json").exists()
    assert not (tmp_path / "last_response_raw.txt").exists()


def test_usage_from_bytes_takes_last_usage():
    raw = b'data: {"usage":{"prompt_tokens":1,"completion_tokens":1}}\n\ndata: {"usage":{"prompt_tokens":5,"completion_tokens":6}}\n\n'
    assert proxy.usage_from_bytes(raw) == (5, 6)
    assert proxy.usage_from_bytes(b"garbage") == (None, None)
