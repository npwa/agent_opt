"""Smoke test of llama-server with the speculative-decoding types (plan item P9).

Loads the target GGUF (the Ollama blob of qwen3.6-32k) once per --spec-type and runs one greedy completion;
reports decode speed and the server's own draft counters. Not a benchmark (one prompt, one run): it shows that
the build, the model and each speculation type work together before M5.3 measures anything.

Binary: LLAMA_SERVER (default: the llama-server bundled with Ollama, the engine the baselines ran on, started with
Ollama's library path). The project-local mainline build (~/work/agent_opt_ad/llama.cpp) cannot load the Ollama blob
of qwen3.6: "key qwen35moe.rope.dimension_sections has wrong array length; expected 4, got 3".

Run from the repository root: PYTHONPATH=. python results/v1/spec_decode/smoke.py <out dir> <spec type> [<spec type> ...]
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

import httpx

from harness import server

LLAMA = Path(os.environ.get("LLAMA_SERVER", "/usr/local/lib/ollama/llama-server"))
ENV = {**os.environ, "LD_LIBRARY_PATH": "/usr/local/lib/ollama:/usr/local/lib/ollama/cuda_v13",
       # without this the bundled llama-server loads only its CPU backend (decode 12.7 instead of 37 tok/s)
       "GGML_BACKEND_PATH": "/usr/local/lib/ollama/cuda_v13/libggml-cuda.so"}
MODEL = Path(server.DEFAULT_MODELS_DIR) / "blobs/sha256-f5ee307a2982106a6eb82b62b2c00b575c9072145a759ae4660378acda8dcf2d"
PORT = 8091
CODE = """#include <stdio.h>
#include <stdlib.h>

static int cmp(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(void) {
    int cap = 16, n = 0, v;
    int *a = malloc(cap * sizeof *a);
    while (scanf("%d", &v) == 1) {
        if (n == cap) { cap *= 2; a = realloc(a, cap * sizeof *a); }
        a[n++] = v;
    }
    qsort(a, n, sizeof *a, cmp);
    for (int i = 0; i < n; i++) printf("%d\\n", a[i]);
    free(a);
    return 0;
}
"""
# three kinds of output: new code, prose, and code that mostly repeats the context (as edit/write calls do)
PROMPTS = {
    "code": "Write a C program that reads integers from standard input, stores them in a dynamic array, sorts them "
            "with quicksort and prints the sorted values and their median. Include comments.\n\n```c\n",
    "prose": "Explain in four paragraphs why speculative decoding speeds up text generation on a GPU, and when it does not.\n\n",
    "copy": "Here is a C file:\n\n```c\n" + CODE + "```\n\nRewrite the file with a comment above main, changing nothing else.\n\n```c\n",
}
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
server.release_vram()
rows = []
for spec in sys.argv[2:]:
    args = [str(LLAMA), "-m", str(MODEL), "-c", "32768", "-np", "1", "--flash-attn", "on", "--port", str(PORT),
            "--no-webui", "--log-verbosity", "3"]
    if spec != "none":
        args += ["--spec-type", spec]
    log = out / f"llama_server_{spec}.log"
    t0 = time.monotonic()
    proc = subprocess.Popen(args, env=ENV, stdout=open(log, "wb"), stderr=subprocess.STDOUT, start_new_session=True)
    row = {"spec_type": spec}
    try:
        while time.monotonic() - t0 < 300:
            try:
                if httpx.get(f"http://127.0.0.1:{PORT}/health", timeout=2).status_code == 200:
                    break
            except httpx.HTTPError:
                pass
            if proc.poll() is not None:
                raise RuntimeError(f"llama-server exited early (see {log})")
            time.sleep(1)
        row["load_s"] = round(time.monotonic() - t0, 1)
        for name, prompt in PROMPTS.items():
            r = httpx.post(f"http://127.0.0.1:{PORT}/completion", timeout=600, json={
                "prompt": prompt, "n_predict": 300, "temperature": 0, "seed": 1, "cache_prompt": False}).json()
            tm = r.get("timings", {})
            row[name] = {k: tm.get(k) for k in ("prompt_n", "predicted_n", "predicted_per_second", "draft_n", "draft_n_accepted")}
            row[name]["text_sha"] = __import__("hashlib").sha256(r.get("content", "").encode()).hexdigest()[:12]
    except Exception as e:  # report, do not hide
        row["error"] = f"{type(e).__name__}: {e}"
    finally:
        import signal
        if proc.poll() is None:
            os.killpg(proc.pid, signal.SIGINT)
            try:
                proc.wait(60)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
    rows.append(row)
    print(json.dumps(row), flush=True)
(out / "smoke.json").write_text(json.dumps(rows, indent=2) + "\n")
