"""
harness/specsweep.py

Speculative-decoding sweep (plan item M5.3) on *replayed agent calls*.

The requests of agent trials recorded by the proxy (``AGENT_OPT_CAPTURE=all``) are replayed, one trial after the other
and in call order, against Ollama's bundled ``llama-server`` (the engine of the baselines) started with one speculation
setting at a time. Sampling is greedy (temperature 0), so a setting that changes the generated text is visible as a
difference to the run without speculation. Per call the server's own timings are kept (decode speed, drafted and
accepted tokens), and every call is classified by what the model produced: a ``write``/``edit`` tool call (code, mostly
copied from the context), another tool call (JSON arguments), or prose (a final answer).

    python -m harness.specsweep run --requests results/v1/m5_specdecode/capture/proxy/requests --out results/v1/m5_specdecode/sweep
    python -m harness.specsweep report --out results/v1/m5_specdecode/sweep
"""

import argparse
import hashlib
import json
import os
import signal
import subprocess
import time
from pathlib import Path

import httpx

from analysis import stats
from harness import server

LLAMA = Path(os.environ.get("LLAMA_SERVER", "/usr/local/lib/ollama/llama-server"))
ENV = {"LD_LIBRARY_PATH": "/usr/local/lib/ollama:/usr/local/lib/ollama/cuda_v13",
       # without this the bundled llama-server loads only its CPU backend (decode 12.7 instead of 37 tok/s)
       "GGML_BACKEND_PATH": "/usr/local/lib/ollama/cuda_v13/libggml-cuda.so"}
MODEL = Path(server.DEFAULT_MODELS_DIR) / "blobs/sha256-f5ee307a2982106a6eb82b62b2c00b575c9072145a759ae4660378acda8dcf2d"
PORT = 8092
CODE_TOOLS = {"write", "edit", "patch", "multiedit"}

# name -> extra llama-server arguments; "none" is the reference, "none_repeat" measures the run-to-run floor
SETTINGS: dict[str, list[str]] = {
    "none": [],
    "none_repeat": [],
    "mtp_n2": ["--spec-type", "draft-mtp", "--spec-draft-n-max", "2"],
    "mtp_n3": ["--spec-type", "draft-mtp"],  # server default draft length 3
    "mtp_n4": ["--spec-type", "draft-mtp", "--spec-draft-n-max", "4"],
    "ngram_mod_n8": ["--spec-type", "ngram-mod", "--spec-ngram-mod-n-max", "8"],
    "ngram_mod_n16": ["--spec-type", "ngram-mod", "--spec-ngram-mod-n-max", "16"],
    "ngram_mod_n64": ["--spec-type", "ngram-mod"],  # server default
    "ngram_map_k": ["--spec-type", "ngram-map-k"],
}
KINDS = ("tool_code", "tool_other", "prose")


def classify(message: dict) -> str:
    """What a reply is: 'tool_code' (write/edit call), 'tool_other' (any other tool call) or 'prose'."""
    calls = message.get("tool_calls") or []
    if not calls:
        return "prose"
    names = {(c.get("function") or {}).get("name", "") for c in calls}
    return "tool_code" if names & CODE_TOOLS else "tool_other"


def text_sha(message: dict) -> str:
    """Fingerprint of everything the model generated in one reply (thinking, text, tool calls)."""
    payload = json.dumps([message.get("reasoning_content"), message.get("content"),
                          [((c.get("function") or {}).get("name"), (c.get("function") or {}).get("arguments"))
                           for c in message.get("tool_calls") or []]], sort_keys=True)
    return hashlib.sha256(payload.encode()).hexdigest()[:16]


def load_trials(root: Path) -> dict[str, list[dict]]:
    """{trial id: request bodies in call order} for every trial directory under ``root``."""
    out = {}
    for d in sorted(p for p in Path(root).iterdir() if p.is_dir()):
        files = sorted(d.glob("a*_*.json"))
        if files:
            out[d.name] = [json.loads(f.read_text()) for f in files]
    return out


def replay_body(body: dict, max_tokens: int) -> dict:
    out = {k: v for k, v in body.items() if k not in ("stream", "stream_options", "seed", "top_p", "temperature")}
    out.update(stream=False, temperature=0, seed=1, max_tokens=min(body.get("max_tokens") or max_tokens, max_tokens))
    return out


def start_server(args: list[str], log: Path, ctx: int = 32768, timeout_s: float = 300) -> subprocess.Popen:
    cmd = [str(LLAMA), "-m", str(MODEL), "-c", str(ctx), "-np", "1", "--flash-attn", "on", "--port", str(PORT),
           "--no-webui", "--jinja", "--log-verbosity", "3", *args]
    proc = subprocess.Popen(cmd, env={**os.environ, **ENV}, stdout=open(log, "wb"), stderr=subprocess.STDOUT, start_new_session=True)
    t0 = time.monotonic()
    while time.monotonic() - t0 < timeout_s:
        try:
            if httpx.get(f"http://127.0.0.1:{PORT}/health", timeout=2).status_code == 200:
                return proc
        except httpx.HTTPError:
            pass
        if proc.poll() is not None:
            raise RuntimeError(f"llama-server exited early (see {log})")
        time.sleep(1)
    stop_server(proc)
    raise RuntimeError("llama-server did not become ready")


def stop_server(proc: subprocess.Popen) -> None:
    if proc.poll() is None:
        os.killpg(proc.pid, signal.SIGINT)
        try:
            proc.wait(60)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)


def run_setting(name: str, trials: dict[str, list[dict]], out: Path, max_tokens: int) -> list[dict]:
    log = out / f"llama_server_{name}.log"
    proc = start_server(SETTINGS[name], log)
    rows = []
    try:
        with httpx.Client(timeout=1200) as client:
            for trial, bodies in trials.items():
                for i, body in enumerate(bodies):
                    r = client.post(f"http://127.0.0.1:{PORT}/v1/chat/completions", json=replay_body(body, max_tokens)).json()
                    if "choices" not in r:
                        rows.append({"setting": name, "trial": trial, "call": i, "error": str(r)[:300]})
                        continue
                    msg, tm = r["choices"][0]["message"], r.get("timings", {})
                    rows.append({"setting": name, "trial": trial, "call": i, "kind": classify(msg), "sha": text_sha(msg),
                                 "finish": r["choices"][0].get("finish_reason"), "prompt_n": tm.get("prompt_n"),
                                 "cache_n": tm.get("cache_n"), "predicted_n": tm.get("predicted_n"),
                                 "predicted_ms": tm.get("predicted_ms"), "draft_n": tm.get("draft_n", 0),
                                 "draft_accepted": tm.get("draft_n_accepted", 0)})
    finally:
        stop_server(proc)
    return rows


def summarise(rows: list[dict], reference: str = "none") -> list[dict]:
    """Per setting and output kind (and 'all'): calls, decode tok/s (pooled), acceptance, share identical to ``reference``."""
    ref = {(r["trial"], r["call"]): r for r in rows if r["setting"] == reference and "error" not in r}
    out = []
    for name in dict.fromkeys(r["setting"] for r in rows):
        mine = [r for r in rows if r["setting"] == name and "error" not in r]
        for kind in (*KINDS, "all"):
            sel = [r for r in mine if kind == "all" or ref.get((r["trial"], r["call"]), r)["kind"] == kind]
            if not sel:
                continue
            tokens, ms = sum(r["predicted_n"] or 0 for r in sel), sum(r["predicted_ms"] or 0 for r in sel)
            drafted, acc = sum(r["draft_n"] or 0 for r in sel), sum(r["draft_accepted"] or 0 for r in sel)
            same = [r["sha"] == ref[(r["trial"], r["call"])]["sha"] for r in sel if (r["trial"], r["call"]) in ref]
            out.append({"setting": name, "kind": kind, "calls": len(sel), "tokens": tokens,
                        "tok_s": round(1000 * tokens / ms, 2) if ms else None,
                        "drafted": drafted, "accepted": acc, "acceptance": round(acc / drafted, 3) if drafted else None,
                        "identical": sum(same), "compared": len(same)})
    base = {(s["kind"]): s["tok_s"] for s in out if s["setting"] == reference}
    for s in out:
        s["speed_vs_ref"] = round(s["tok_s"] / base[s["kind"]], 3) if s["tok_s"] and base.get(s["kind"]) else None
    return out


def render(summary: list[dict]) -> str:
    L = ["# Speculative decoding on replayed agent calls", "",
         "Greedy replay of recorded agent requests against Ollama's bundled `llama-server`, 32k context, one slot, flash attention on. "
         "`speed` = pooled decode tokens/s relative to `none`; `identical` = replies whose full text (thinking, content, tool calls) "
         "equals the reply without speculation; `none_repeat` is the same setting run a second time (the floor for identity).", "",
         "| Setting | Output | calls | tok/s | speed | drafted | accepted | acceptance | identical |", "|---|---|---:|---:|---:|---:|---:|---:|---|"]
    for s in summary:
        lo_hi = ""
        if s["compared"]:
            lo, hi = stats.wilson_interval(s["identical"], s["compared"])
            lo_hi = f"{s['identical']}/{s['compared']} [{lo:.0%}, {hi:.0%}]"
        acc = f"{s['acceptance']:.0%}" if s["acceptance"] is not None else ""
        spd = f"{s['speed_vs_ref']:.2f}" if s["speed_vs_ref"] else ""
        L.append(f"| {s['setting']} | {s['kind']} | {s['calls']} | {s['tok_s']} | {spd} | {s['drafted']} | {s['accepted']} | {acc} | {lo_hi} |")
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Speculative-decoding sweep on replayed agent calls")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--requests", type=Path, required=True, help="proxy/requests directory with one subdirectory per trial")
    r.add_argument("--settings", nargs="+", default=list(SETTINGS))
    r.add_argument("--max-tokens", type=int, default=1500, help="cap on generated tokens per call")
    r.add_argument("--out", type=Path, required=True)
    p = sub.add_parser("report")
    p.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    rows_path = a.out / "calls.jsonl"
    if a.cmd == "run":
        trials = load_trials(a.requests)
        server.release_vram()
        done = {json.loads(line)["setting"] for line in rows_path.read_text().splitlines()} if rows_path.exists() else set()
        for name in a.settings:
            if name in done:
                continue
            t0 = time.monotonic()
            rows = run_setting(name, trials, a.out, a.max_tokens)
            with open(rows_path, "a") as f:
                f.writelines(json.dumps(r) + "\n" for r in rows)
            print(f"{name}: {len(rows)} calls in {time.monotonic() - t0:.0f} s", flush=True)
    rows = [json.loads(line) for line in rows_path.read_text().splitlines()]
    summary = summarise(rows)
    (a.out / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    (a.out / "RESULTS.md").write_text(render(summary))
    print(render(summary))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
