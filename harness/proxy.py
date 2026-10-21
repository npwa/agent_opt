#!/usr/bin/env python3
"""
harness/proxy.py (ported from benchmark/metrics_proxy.py, behaviour-preserving;
paths and upstream now come from AGENT_OPT_DATA_DIR / OLLAMA_BASE)

A transparent logging proxy for OpenCode -> Ollama traffic.

Point OpenCode's provider baseURL at this proxy instead of Ollama directly:
    Ollama native:   http://localhost:11434/v1
    Through proxy:   http://localhost:11435/v1

Default mode (AGENT_OPT_PASSTHROUGH=1): a passive observer. Request and response
bytes are forwarded untouched; the proxy only timestamps chunks and, after the stream
ends, reads Ollama's own usage fields for token counts. Nothing is injected, so the
request seen by Ollama is byte-identical to the client's (the class of bugs behind the
three instrumentation failures documented in the v0 notes cannot occur).

Legacy mode (AGENT_OPT_PASSTHROUGH=0): the v0 behaviour (forces stream=true, re-chunks
the SSE stream line by line, writes last_request/last_response debug dumps). Kept only to
reproduce v0 numbers.

Request capture (AGENT_OPT_CAPTURE, off by default; passthrough mode only): "all", or a comma-separated
list of substrings of trial ids to capture. The raw request body of every chat call of a captured trial is
written to <data dir>/requests/<trial_id>/a<attempt>_<seq>.json *after* the response stream has ended, so
the forwarded bytes and the timings are not affected; the metrics record gets a ``request_file`` field.
These bodies are what harness/promptreplay.py replays (cold/warm TTFT of prompt variants without the agent).

Trial identification: clients may send X-Trial-Id / X-Task-Id headers (set through the
OpenCode provider options). They are logged and stripped before forwarding; the task
falls back to the current_task.txt label file.

Requires: pip install fastapi uvicorn httpx
Run:      python metrics_proxy.py
"""

import json
import os
import re
import time
from pathlib import Path

import httpx
from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse

OLLAMA_BASE = os.environ.get("OLLAMA_BASE", "http://localhost:11434")
PASSTHROUGH = os.environ.get("AGENT_OPT_PASSTHROUGH", "1") != "0"
OWN_HEADERS = ("x-trial-id", "x-task-id", "x-attempt")
HOP_HEADERS = ("host", "content-length", "accept-encoding", "connection", "transfer-encoding")
RESP_DROP_HEADERS = ("content-length", "transfer-encoding", "content-encoding", "connection")
CAPTURE = os.environ.get("AGENT_OPT_CAPTURE", "")
DATA_DIR = Path(os.environ.get("AGENT_OPT_DATA_DIR", Path.cwd()))
LOG_PATH = DATA_DIR / "metrics.jsonl"
TASK_LABEL_PATH = DATA_DIR / "current_task.txt"

app = FastAPI()
client = httpx.AsyncClient(timeout=None)


def current_task_label() -> str:
    """Reads a label you set before each benchmark task, e.g.:
    echo 'coding_01_my_sqrt_review' > current_task.txt
    Falls back to 'unlabeled' if the file isn't present."""
    try:
        return TASK_LABEL_PATH.read_text().strip() or "unlabeled"
    except FileNotFoundError:
        return "unlabeled"


_SEQ: dict[tuple[str, str], int] = {}


def capture_wanted(trial_id: str | None) -> bool:
    spec = CAPTURE.strip()
    if not spec or not trial_id:
        return False
    return spec == "all" or any(s and s in trial_id for s in spec.split(","))


def capture_request(trial_id: str, attempt: str | None, body: bytes) -> str:
    """Write the raw request body of one call; returns its path relative to the data directory."""
    key = (trial_id, attempt or "1")
    seq = _SEQ[key] = _SEQ.get(key, 0) + 1
    rel = f"requests/{trial_id}/a{key[1]}_{seq:03d}.json"
    path = DATA_DIR / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(body)
    return rel


def log_record(record: dict) -> None:
    with open(LOG_PATH, "a") as f:
        f.write(json.dumps(record) + "\n")


def usage_from_bytes(raw: bytes) -> tuple[int | None, int | None]:
    """Last usage object in an SSE stream or a plain JSON body -> (prompt, completion) tokens."""
    prompt = completion = None
    text = raw.decode("utf-8", errors="replace")
    candidates = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("data:") and "[DONE]" not in line:
            candidates.append(line[len("data:"):].strip())
    if not candidates:
        candidates = [text]
    for cand in candidates:
        try:
            usage = json.loads(cand).get("usage")
        except (json.JSONDecodeError, AttributeError):
            continue
        if usage:
            prompt = usage.get("prompt_tokens", prompt)
            completion = usage.get("completion_tokens", completion)
    return prompt, completion


def request_params(body: bytes) -> dict:
    """Sampling-relevant request fields, observed read-only (the body is never modified)."""
    try:
        data = json.loads(body)
    except (json.JSONDecodeError, UnicodeDecodeError):
        return {}
    if not isinstance(data, dict):
        return {}
    keys = ("model", "temperature", "top_p", "top_k", "seed", "max_tokens", "stream")
    return {k: data[k] for k in keys if k in data}


def approx_completion_tokens(raw: bytes) -> int:
    text = raw.decode("utf-8", errors="replace")
    return len(" ".join(re.findall(r'"content":"(.*?)"(?<!\\)', text)).split())


async def passthrough(path: str, request: Request):
    body = await request.body()
    is_chat = "chat/completions" in path
    trial_id = request.headers.get("x-trial-id")
    attempt = request.headers.get("x-attempt")
    task = request.headers.get("x-task-id") or current_task_label()
    req_params = request_params(body) if is_chat else {}
    headers = {k: v for k, v in request.headers.items()
               if k.lower() not in HOP_HEADERS + OWN_HEADERS}
    url = f"{OLLAMA_BASE}/{path}"
    if request.url.query:
        url += f"?{request.url.query}"
    t_start = time.monotonic()
    wall_start = time.time()
    upstream = await client.send(
        client.build_request(request.method, url, content=body, headers=headers), stream=True)
    out_headers = {k: v for k, v in upstream.headers.items() if k.lower() not in RESP_DROP_HEADERS}

    async def relay():
        t_first = None
        n_chunks = 0
        chunk_t: list[float] = []  # arrival offsets (s since request start), for inter-token latency
        raw = bytearray()
        finished = False
        try:
            async for chunk in upstream.aiter_raw():
                if t_first is None and b"data:" in chunk and b"[DONE]" not in chunk:
                    t_first = time.monotonic()
                n_chunks += 1
                if is_chat:
                    chunk_t.append(round(time.monotonic() - t_start, 3))
                    raw += chunk
                yield chunk
            finished = True
        finally:
            await upstream.aclose()
            if is_chat:
                t_end = time.monotonic()
                prompt, completion = usage_from_bytes(bytes(raw))
                approx = completion is None
                if approx:
                    completion = approx_completion_tokens(bytes(raw))
                rec = {
                    "task": task, "trial_id": trial_id, "attempt": attempt,
                    "wall_start": wall_start, "req": req_params,
                    "ttft_s": None if t_first is None else round(t_first - t_start, 4),
                    "total_s": round(t_end - t_start, 4),
                    "prompt_tokens": prompt, "completion_tokens": completion,
                    "completion_tokens_is_approx": approx,
                    "status": upstream.status_code, "chunks": n_chunks, "chunk_t": chunk_t,
                    "aborted": not finished,
                }
                if rec["ttft_s"] and prompt:
                    rec["input_tok_s"] = round(prompt / rec["ttft_s"], 2)
                if completion and t_first and t_end > t_first:
                    rec["output_tok_s"] = round(completion / (t_end - t_first), 2)
                if capture_wanted(trial_id):
                    rec["request_file"] = capture_request(trial_id, attempt, body)
                log_record(rec)

    return StreamingResponse(relay(), status_code=upstream.status_code, headers=out_headers)


@app.api_route("/{path:path}", methods=["GET", "POST", "HEAD"])
async def proxy(path: str, request: Request):
    if PASSTHROUGH:
        return await passthrough(path, request)
    return await legacy(path, request)


async def legacy(path: str, request: Request):
    body_bytes = await request.body()
    is_chat = "chat/completions" in path

    # NOTE: we previously also forced stream_options.include_usage=true here
    # to get exact token counts. That appears to trigger a change in how
    # Ollama's OpenAI-compat layer splits reasoning vs. content for
    # reasoning-capable models (all text ends up in "reasoning", "content"
    # stays empty) — confirmed by comparing proxied vs. unproxied runs of
    # the same model/task. Removed; token counts fall back to the
    # approximate whitespace-based estimate instead. If you need exact
    # token counts back, re-add stream_options here, but verify content
    # still populates correctly for your model before trusting benchmark
    # results run with it enabled.
    if is_chat and body_bytes:
        try:
            body = json.loads(body_bytes)
            body["stream"] = True
            body_bytes = json.dumps(body).encode()
        except json.JSONDecodeError:
            pass

    url = f"{OLLAMA_BASE}/{path}"
    task_label = request.headers.get("x-task-id") or current_task_label()
    trial_id = request.headers.get("x-trial-id")
    t_start = time.monotonic()
    ts_start_wall = time.time()

    # Debug aid: dump the outgoing request body so we can inspect exactly
    # what was sent to Ollama (e.g. whether a "tools" array is present).
    if is_chat:
        try:
            with open(DATA_DIR / "last_request_raw.json", "wb") as f:
                f.write(body_bytes)
        except Exception:
            pass

    # Drop headers that no longer match body_bytes once we've modified the
    # body (stream flag / stream_options injected above). httpx recomputes
    # Content-Length itself from the content= we pass it; forwarding the
    # client's original Content-Length causes a mismatch and the request
    # fails with "Too much data for declared Content-Length".
    forward_headers = {
        k: v for k, v in request.headers.items()
        if k.lower() not in ("host", "content-length") + OWN_HEADERS
    }

    if not is_chat:
        # Pass through non-chat calls (models list, embeddings, etc.) unlogged.
        resp = await client.request(
            request.method, url, content=body_bytes, headers=forward_headers,
        )
        return StreamingResponse(iter([resp.content]), status_code=resp.status_code,
                                  media_type=resp.headers.get("content-type"))

    t_first_token = None
    prompt_tokens = None
    completion_tokens = None
    chunks = []

    async def stream_and_capture():
        nonlocal t_first_token, prompt_tokens, completion_tokens
        async with client.stream(
            "POST", url, content=body_bytes, headers=forward_headers,
        ) as resp:
            async for raw_line in resp.aiter_lines():
                # SSE requires a blank line to terminate each event. Forward
                # every line, blank or not, so the client's stream parser
                # sees valid event boundaries — only skip the token-capture
                # logic (not the forwarding) for blank lines.
                if raw_line:
                    if t_first_token is None and raw_line.startswith("data:") and "[DONE]" not in raw_line:
                        t_first_token = time.monotonic()
                    if raw_line.startswith("data:") and "[DONE]" not in raw_line:
                        try:
                            payload = json.loads(raw_line[len("data:"):].strip())
                            usage = payload.get("usage")
                            if usage:
                                prompt_tokens = usage.get("prompt_tokens", prompt_tokens)
                                completion_tokens = usage.get("completion_tokens", completion_tokens)
                        except json.JSONDecodeError:
                            pass
                    chunks.append(raw_line)
                yield (raw_line + "\n").encode()

        t_end = time.monotonic()

        # Debug aid: dump the full raw response for inspection. Overwrites
        # each call, so it always holds the most recent request's response.
        try:
            with open(DATA_DIR / "last_response_raw.txt", "w") as f:
                f.write("\n".join(chunks))
        except Exception:
            pass

        # Fallback approximation if Ollama's build doesn't return usage yet.
        approx_completion = None
        if completion_tokens is None:
            text_join = " ".join(re.findall(r'"content":"(.*?)"(?<!\\)', " ".join(chunks)))
            approx_completion = len(text_join.split())

        record = {
            "task": task_label,
            "trial_id": trial_id,
            "wall_start": ts_start_wall,
            "ttft_s": None if t_first_token is None else round(t_first_token - t_start, 4),
            "total_s": round(t_end - t_start, 4),
            "prompt_tokens": prompt_tokens,
            "completion_tokens": completion_tokens if completion_tokens is not None else approx_completion,
            "completion_tokens_is_approx": completion_tokens is None,
        }
        if record["ttft_s"] and record["prompt_tokens"]:
            record["input_tok_s"] = round(record["prompt_tokens"] / record["ttft_s"], 2)
        if record["completion_tokens"] and t_first_token:
            gen_time = t_end - t_first_token
            if gen_time > 0:
                record["output_tok_s"] = round(record["completion_tokens"] / gen_time, 2)
        log_record(record)

    return StreamingResponse(stream_and_capture(), media_type="text/event-stream")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=11435)
