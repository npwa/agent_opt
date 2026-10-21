"""
harness/promptreplay.py

Prompt replay (plan item P7): measure the prefill cost of prompt variants *without running the agent*.

Request bodies captured by the proxy (``AGENT_OPT_CAPTURE``, see harness/proxy.py) are replayed against
a server one call after the other, with ``max_tokens`` capped (default 1), and the time to the first
streamed chunk is recorded for every call. Variants transform the recorded bodies (drop tool schemas,
strip the skills block ...), so a prompt-trimming lever can be timed in minutes instead of hours.

Each variant is measured in two cache states, interleaved with the other variants and repeated:

* ``cold``  no cached prefix can match -> a first call in a new project. Default ``--flush nonce`` puts a
            random marker at the start of the system message *and* of the first tool description (about 10
            tokens), so it does not matter whether the server renders tools or system text first or how many
            cache slots it keeps. Other modes: ``dummy`` (an unrelated long prompt), ``unload`` (unload and
            reload the model), ``none``. Measured on this box: ``dummy`` does **not** reliably evict the cache
            (the server keeps more than one prefix), so it is not the default.
* ``warm``  the first request is primed (unmeasured) with the per-trial working-directory path changed, then
            the sequence is replayed with a different path (``--no-perturb`` keeps bodies identical) -> a
            first call right after another trial, as in the back-to-back runs

Caveat of the fast mode: with ``max_tokens`` = 1 the cache holds one generated token, not the recorded
reply, so each follow-up call re-processes its previous reply (~100-200 tokens, a few tenths of a second)
that a real run would find cached. ``--faithful`` keeps the original ``max_tokens`` (decodes everything).

    python -m harness.promptreplay run --requests results/v1/replay_probe/requests/<trial> \\
        --base-url http://localhost:11434/v1 --variants default drop_tools=task+todowrite+skill+webfetch strip_skills
"""

import argparse
import json
import random
import re
import statistics as st
import time
from collections.abc import Callable
from pathlib import Path

import httpx

from analysis import stats
from harness.proxy import usage_from_bytes

DATA_RE = re.compile(rb"data:\s*(?!\[DONE\])\S")  # a data line that is not the terminator
PATH_RE = re.compile(r"/tmp/agent_opt_scratch/([^/\s\"\\]+)/")
SKILLS_MARK = "Skills provide specialized instructions"
FILLER = ("The quick brown fox jumps over the lazy dog while seven quiet engineers measure prefill latency "
          "on a ten gigabyte graphics card. ")


# ---- variants (pure transformations of a request body) -----------------------------------------------------------

def _system_indexes(body: dict) -> list[int]:
    return [i for i, m in enumerate(body.get("messages", [])) if m.get("role") == "system" and isinstance(m.get("content"), str)]


def drop_tools(body: dict, names: set[str]) -> dict:
    out = json.loads(json.dumps(body))
    if "tools" in out:
        out["tools"] = [t for t in out["tools"] if t["function"]["name"] not in names]
        if not out["tools"]:
            out.pop("tools")
            out.pop("tool_choice", None)
    return out


def keep_tools(body: dict, names: set[str]) -> dict:
    out = json.loads(json.dumps(body))
    if "tools" in out:
        out["tools"] = [t for t in out["tools"] if t["function"]["name"] in names]
    return out


def strip_skills(body: dict) -> dict:
    """Remove the skills block (and anything after it) from the system message."""
    out = json.loads(json.dumps(body))
    for i in _system_indexes(out):
        c = out["messages"][i]["content"]
        j = c.find(SKILLS_MARK)
        if j >= 0:
            out["messages"][i]["content"] = c[:j].rstrip() + "\n"
    return out


def make_variant(spec: str) -> Callable[[dict], dict]:
    """'default' | 'drop_tools=a+b' | 'keep_tools=a+b' | 'strip_skills' -> transformation."""
    name, _, arg = spec.partition("=")
    names = {x for x in arg.split("+") if x}
    if name == "default":
        return lambda b: b
    if name == "drop_tools" and names:
        return lambda b: drop_tools(b, names)
    if name == "keep_tools" and names:
        return lambda b: keep_tools(b, names)
    if name == "strip_skills":
        return strip_skills
    raise ValueError(f"unknown variant {spec!r} (default, drop_tools=a+b, keep_tools=a+b, strip_skills)")


def add_nonce(body: dict, nonce: str) -> dict:
    """Prefix the system message and the first tool description with a marker so no cached prefix matches."""
    out = json.loads(json.dumps(body))
    mark = f"[replay {nonce}] "
    idx = _system_indexes(out) or [0]
    if out.get("messages"):
        m = out["messages"][idx[0]]
        if isinstance(m.get("content"), str):
            m["content"] = mark + m["content"]
    if out.get("tools"):
        f = out["tools"][0]["function"]
        f["description"] = mark + f.get("description", "")
    return out


def perturb(body: dict, tag: str) -> dict:
    """Change the per-trial scratch directory in a body (a new trial has a new working directory)."""
    text = PATH_RE.sub(lambda m: f"/tmp/agent_opt_scratch/{tag}/", json.dumps(body))
    return json.loads(text)


# ---- one replay ----------------------------------------------------------------------------------------------------

def load_sequence(path: Path) -> list[dict]:
    """Request bodies of one captured trial, in call order (directory of a*_NNN.json files, or one file)."""
    files = [Path(path)] if Path(path).is_file() else sorted(Path(path).glob("a*_*.json"))
    if not files:
        raise FileNotFoundError(f"no captured requests under {path}")
    return [json.loads(f.read_text()) for f in files]


def post_timed(client: httpx.Client, url: str, body: dict) -> dict:
    """POST a streaming chat request; seconds to the first data chunk, total, and Ollama's usage counts."""
    t0 = time.monotonic()
    t_first = None
    raw = bytearray()
    with client.stream("POST", url, json=body) as r:
        status = r.status_code
        for chunk in r.iter_raw():
            if t_first is None and DATA_RE.search(chunk):
                t_first = time.monotonic()
            raw += chunk
    t1 = time.monotonic()
    prompt, completion = usage_from_bytes(bytes(raw))
    return {"status": status, "ttft_s": None if t_first is None else t_first - t0, "total_s": t1 - t0,
            "prompt_tokens": prompt, "completion_tokens": completion}


def prepare(body: dict, max_tokens: int | None, model: str | None) -> dict:
    out = dict(body)
    out["stream"] = True
    out["stream_options"] = {"include_usage": True}
    if max_tokens is not None:
        out["max_tokens"] = max_tokens
    if model:
        out["model"] = model
    return out


def flush_cache(client: httpx.Client, base_url: str, model: str, mode: str, tokens: int = 1500) -> None:
    """Make the server forget the cached prefix. 'dummy': an unrelated prompt overwrites it; 'unload': also
    unload the model and pre-load it with a tiny prompt (so loading time never lands in a measured call)."""
    if mode == "none":
        return
    chat = f"{base_url}/chat/completions"
    if mode == "unload":
        client.post(base_url.removesuffix("/v1") + "/api/generate", json={"model": model, "keep_alive": 0})
        post_timed(client, chat, {"model": model, "messages": [{"role": "user", "content": "hi"}],
                                  "stream": True, "max_tokens": 1})
    filler = (FILLER * (tokens // 20 + 1))[: tokens * 5]
    post_timed(client, chat, {"model": model, "messages": [{"role": "user", "content": f"[{random.random()}] {filler}"}],
                              "stream": True, "stream_options": {"include_usage": True}, "max_tokens": 1})


def run_sequence(client: httpx.Client, base_url: str, bodies: list[dict], max_tokens: int | None, model: str | None) -> list[dict]:
    return [post_timed(client, f"{base_url}/chat/completions", prepare(b, max_tokens, model)) for b in bodies]


# ---- experiment ------------------------------------------------------------------------------------------------------

def experiment(client: httpx.Client, base_url: str, bodies: list[dict], variants: list[str], repeats: int = 3,
               flush: str = "nonce", max_tokens: int | None = 1, model: str | None = None, do_perturb: bool = True,
               seed: int = 0, log: Callable[[str], None] = lambda s: None) -> list[dict]:
    """Interleaved A/B/C... replays. Returns one row per (variant, mode, repeat) with per-call timings."""
    rng = random.Random(seed)
    model = model or bodies[0].get("model", "")
    transforms = {v: make_variant(v) for v in variants}
    chat = f"{base_url}/chat/completions"
    rows = []
    for rep in range(repeats):
        order = variants[:]
        rng.shuffle(order)
        for v in order:
            vb = [transforms[v](b) for b in bodies]
            # cold: nothing cached can match
            if flush == "nonce":
                nonce = f"{rng.getrandbits(48):012x}"
                cold_bodies = [add_nonce(b, nonce) for b in vb]  # one marker per sequence: follow-ups still hit the cache
            else:
                flush_cache(client, base_url, model, flush)
                cold_bodies = vb
            res = run_sequence(client, base_url, cold_bodies, max_tokens, model)
            rows.append({"variant": v, "mode": "cold", "repeat": rep, "calls": res})
            log(f"rep {rep} {v} cold: first call {res[0]['ttft_s']:.2f} s")
            # warm: prime the real prefix (unmeasured), then replay as a "next trial" (new working directory)
            primed = [perturb(b, f"replay_{rep}_{v}_a") for b in vb] if do_perturb else vb
            if flush == "nonce":  # the cold run used a marker, so the unmarked prefix is not cached yet
                post_timed(client, chat, prepare(primed[0], max_tokens, model))
            wb = [perturb(b, f"replay_{rep}_{v}_b") for b in vb] if do_perturb else vb
            res = run_sequence(client, base_url, wb, max_tokens, model)
            rows.append({"variant": v, "mode": "warm", "repeat": rep, "calls": res})
            log(f"rep {rep} {v} warm: first call {res[0]['ttft_s']:.2f} s")
    return rows


def summarise(rows: list[dict]) -> list[dict]:
    """Median first-call and whole-sequence ttft per (variant, cache state); ratio to `default` (or the first variant)."""
    variants = sorted(dict.fromkeys(r["variant"] for r in rows), key=lambda v: v != "default")  # default first
    out = []
    for mode in ("cold", "warm"):
        base = None
        for v in variants:
            rs = [r for r in rows if r["variant"] == v and r["mode"] == mode]
            first = [r["calls"][0]["ttft_s"] for r in rs]
            total = [sum(c["ttft_s"] for c in r["calls"]) for r in rs]
            row = {"variant": v, "mode": mode, "repeats": len(rs),
                   "first_call_prompt_tokens": rs[0]["calls"][0]["prompt_tokens"],
                   "first_call_ttft_s": st.median(first), "sequence_ttft_s": st.median(total),
                   "calls": len(rs[0]["calls"]), "first_call_samples": first, "sequence_samples": total}
            if base is None:
                base = row
            elif len(rs) >= 2:
                cmp = stats.compare_samples(base["sequence_samples"], total, resamples=2000)
                row["vs_default_sequence_ratio"] = cmp["ratio_median"]
                row["vs_default_sequence_ratio_ci"] = cmp["ratio_ci"]
                row["directional"] = cmp["directional"]
            out.append(row)
    return out


def render(summary: list[dict], meta: dict) -> str:
    L = [f"# Prompt replay: {meta['sequence']}", "",
         f"{meta['calls']} recorded calls, {meta['repeats']} interleaved repeats per variant, flush = `{meta['flush']}`, "
         f"max_tokens = {meta['max_tokens']}; model {meta['model']}. Times are time-to-first-token (seconds).", "",
         "| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |",
         "|---|---|---:|---:|---:|---|"]
    for r in summary:
        vs = ""
        if "vs_default_sequence_ratio" in r:
            lo, hi = r["vs_default_sequence_ratio_ci"]
            vs = f"x{r['vs_default_sequence_ratio']:.2f} [{lo:.2f}, {hi:.2f}]" + (" (directional, n < 5)" if r["directional"] else "")
        L.append(f"| {r['variant']} | {r['mode']} | {r['first_call_prompt_tokens']} | {r['first_call_ttft_s']:.2f} | "
                 f"{r['sequence_ttft_s']:.2f} | {vs} |")
    L += ["", "Cold = cache flushed before the sequence; warm = the sequence replayed right after another one "
          "(working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring)."]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Replay captured request bodies and time prompt variants")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--requests", type=Path, required=True, help="directory of a*_NNN.json files of one trial (or one file)")
    r.add_argument("--base-url", default="http://localhost:11434/v1")
    r.add_argument("--model", help="override the model of the recorded bodies")
    r.add_argument("--variants", nargs="+", default=["default"])
    r.add_argument("--repeats", type=int, default=3)
    r.add_argument("--flush", choices=["nonce", "none", "dummy", "unload"], default="nonce",
                   help="how the cold state is made (default nonce: a marker at the start of the prompt)")
    r.add_argument("--max-tokens", type=int, default=1)
    r.add_argument("--faithful", action="store_true", help="keep the recorded max_tokens (decode everything)")
    r.add_argument("--no-perturb", action="store_true", help="warm replays use identical bodies")
    r.add_argument("--seed", type=int, default=0)
    r.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    bodies = load_sequence(a.requests)
    for v in a.variants:
        make_variant(v)  # validate before talking to the server
    base = a.base_url.rstrip("/")
    max_tokens = None if a.faithful else a.max_tokens
    with httpx.Client(timeout=None) as client:
        rows = experiment(client, base, bodies, a.variants, a.repeats, a.flush, max_tokens, a.model,
                          not a.no_perturb, a.seed, log=print)
    summary = summarise(rows)
    meta = {"sequence": str(a.requests), "calls": len(bodies), "repeats": a.repeats, "flush": a.flush,
            "max_tokens": "recorded" if a.faithful else max_tokens, "model": a.model or bodies[0].get("model", "")}
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "promptreplay.json").write_text(json.dumps({"meta": meta, "summary": summary, "runs": rows}, indent=2) + "\n")
    (a.out / "promptreplay.md").write_text(render(summary, meta))
    print(render(summary, meta))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
