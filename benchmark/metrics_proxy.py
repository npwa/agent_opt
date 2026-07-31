#!/usr/bin/env python3
"""
metrics_proxy.py

A transparent logging proxy for OpenCode -> Ollama traffic.

Point OpenCode's provider baseURL at this proxy instead of Ollama directly:
    Ollama native:   http://localhost:11434/v1
    Through proxy:   http://localhost:11435/v1

The proxy forwards every request unchanged (plus enabling stream usage
reporting) to Ollama, and logs one JSON line per request to metrics.jsonl
with timing and token-count data.

Requires: pip install fastapi uvicorn httpx
Run:      python metrics_proxy.py
"""

import json
import time
import re
from pathlib import Path

import httpx
from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse

OLLAMA_BASE = "http://localhost:11434"
LOG_PATH = Path(__file__).parent / "metrics.jsonl"
TASK_LABEL_PATH = Path(__file__).parent / "current_task.txt"

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


def log_record(record: dict) -> None:
    with open(LOG_PATH, "a") as f:
        f.write(json.dumps(record) + "\n")


@app.api_route("/{path:path}", methods=["GET", "POST", "HEAD"])
async def proxy(path: str, request: Request):
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
    task_label = current_task_label()
    t_start = time.monotonic()
    ts_start_wall = time.time()

    # Debug aid: dump the outgoing request body so we can inspect exactly
    # what was sent to Ollama (e.g. whether a "tools" array is present).
    if is_chat:
        try:
            with open(Path(__file__).parent / "last_request_raw.json", "wb") as f:
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
        if k.lower() not in ("host", "content-length")
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
            with open(Path(__file__).parent / "last_response_raw.txt", "w") as f:
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
