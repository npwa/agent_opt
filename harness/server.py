"""
harness/server.py

Per-configuration Ollama server settings (plan item P4, factors of F11).

A run configuration may carry a ``server:`` mapping, for example::

    id: ctx16_q8
    model: ollama/qwen3.6-16k
    server:
      num_ctx: 16384          # context window: the model is derived from base_model with this num_ctx
      base_model: qwen3.6     # optional, default qwen3.6
      kv_cache_type: q8_0     # f16 | q8_0 | q4_0   (OLLAMA_KV_CACHE_TYPE)
      flash_attention: true   # OLLAMA_FLASH_ATTENTION
      num_parallel: 1         # OLLAMA_NUM_PARALLEL
      fit_target_mib: 512     # LLAMA_ARG_FIT_TARGET: free VRAM llama.cpp keeps when it decides how many
                              # layers' experts stay on the GPU (default about 1.9 GB)

Such a configuration runs on a **private** ``ollama serve`` (127.0.0.1:11437, same model store) started
by the runner with exactly these settings, so the sweep never depends on the unknown environment of the
system service. Everything else (the proxy, OpenCode, the trials) is unchanged: the runner points the
proxy's upstream (``upstream.txt`` in the proxy data directory) and the uninstrumented parity trials at
the private server for the duration of the configuration's block of trials.

What the server actually did is read back from its log and API and stored in ``platform.yaml`` (layers
offloaded, expert layers overflowing to the CPU, KV cache size and types, flash attention, compute
buffer, prompt-cache limit): a requested setting is not evidence that it took effect.

Findings that shaped this (Ollama 0.32 / llama.cpp, qwen3.6 35B-A3B on a 10 GB card): the llama.cpp
"fit" step decides how many layers' experts stay on the GPU from the free VRAM, so context size matters
through that decision rather than through the KV cache, which is tiny for this hybrid model (10 of 41
layers use attention: 340 MiB at q8_0 for 32k); the server keeps a host-RAM prompt cache (8 GiB) and context
checkpoints, which is why a flushed-looking cache can still serve a prefix.
"""

import json
import os
import re
import signal
import subprocess
import time
from pathlib import Path

import httpx
import yaml

SERVER_KEYS = {"num_ctx", "base_model", "kv_cache_type", "flash_attention", "num_parallel", "fit_target_mib"}
KV_TYPES = ("f16", "q8_0", "q4_0")
PRIVATE_HOST = "127.0.0.1:11437"
SERVICE_URL = "http://localhost:11434"
DEFAULT_MODELS_DIR = "/usr/share/ollama/.ollama/models"
DEFAULT_BASE_MODEL = "qwen3.6"


# ---- settings ------------------------------------------------------------------------------------------------------

def validate_server(data) -> list[str]:
    if not isinstance(data, dict):
        return ["server: must be a mapping"]
    errs = [f"server: unknown key '{k}' (allowed: {sorted(SERVER_KEYS)})" for k in data if k not in SERVER_KEYS]
    n = data.get("num_ctx")
    if "num_ctx" in data and not (isinstance(n, int) and not isinstance(n, bool) and 1024 <= n <= 1_048_576):
        errs.append("server.num_ctx: must be an integer between 1024 and 1048576")
    if "kv_cache_type" in data and data["kv_cache_type"] not in KV_TYPES:
        errs.append(f"server.kv_cache_type: must be one of {list(KV_TYPES)}")
    if "flash_attention" in data and not isinstance(data["flash_attention"], bool):
        errs.append("server.flash_attention: must be true or false")
    p = data.get("num_parallel")
    if "num_parallel" in data and not (isinstance(p, int) and not isinstance(p, bool) and 1 <= p <= 16):
        errs.append("server.num_parallel: must be an integer between 1 and 16")
    f = data.get("fit_target_mib")
    if "fit_target_mib" in data and not (isinstance(f, int) and not isinstance(f, bool) and 0 <= f <= 8192):
        errs.append("server.fit_target_mib: must be an integer between 0 and 8192")
    if "base_model" in data and not (isinstance(data["base_model"], str) and data["base_model"].strip()):
        errs.append("server.base_model: must be a model name")
    return errs


def group_key(server: dict | None) -> str:
    """Configurations with the same key can share a running server ('system' = no managed server)."""
    return "system" if server is None else json.dumps(server, sort_keys=True)


def server_env(server: dict, host: str = PRIVATE_HOST, models_dir: str = DEFAULT_MODELS_DIR) -> dict[str, str]:
    """Environment of the private ``ollama serve`` (on top of the caller's own)."""
    env = {"OLLAMA_HOST": host, "OLLAMA_MODELS": models_dir}
    if "kv_cache_type" in server:
        env["OLLAMA_KV_CACHE_TYPE"] = server["kv_cache_type"]
    if "flash_attention" in server:
        env["OLLAMA_FLASH_ATTENTION"] = "1" if server["flash_attention"] else "0"
    if "num_parallel" in server:
        env["OLLAMA_NUM_PARALLEL"] = str(server["num_parallel"])
    if "fit_target_mib" in server:
        env["LLAMA_ARG_FIT_TARGET"] = str(server["fit_target_mib"])
    return env


# ---- reading back what the server did -----------------------------------------------------------------------------

def _f(rx: str, text: str, cast=float, group: int = 1):
    m = re.findall(rx, text)
    if not m:
        return None
    last = m[-1]
    return cast(last if isinstance(last, str) else last[group - 1])


def parse_server_log(text: str) -> dict:
    """Facts about the most recent model load in an ``ollama serve`` log (None where a line is absent)."""
    cfg_line = next((ln for ln in text.splitlines() if 'msg="server config"' in ln), "")
    cmd_line = next((ln for ln in reversed(text.splitlines()) if 'msg="starting llama-server"' in ln), "")
    start = text.rfind('msg="starting llama-server"')
    seg = text[start:] if start >= 0 else text  # the last load only
    fit = re.findall(r"CUDA0 \([^)]*\): (\d+) layers(?: \((\d+) overflowing\))?,\s+(\d+) MiB used,\s+(\d+) MiB free", seg)
    kv = re.findall(r"llama_kv_cache: size =\s+([\d.]+) MiB \(\s*(\d+) cells,\s+(\d+) layers.*?K \((\w+)\).*?V \((\w+)\)", seg)
    offload = re.findall(r"load_tensors: offloaded (\d+)/(\d+) layers to GPU", seg)
    ckpt = re.findall(r"context checkpoints enabled, max = (\d+), min spacing = (\d+)", seg)
    return {
        "env": {k: _f(rf"{k}:(\S*)", cfg_line, str) for k in ("OLLAMA_FLASH_ATTENTION", "OLLAMA_KV_CACHE_TYPE",
                                                             "OLLAMA_NUM_PARALLEL", "OLLAMA_CONTEXT_LENGTH",
                                                             "LLAMA_ARG_FIT_TARGET")},
        "llama_server_args": {"ctx": _f(r" -c (\d+)", cmd_line, int), "parallel": _f(r" -np (\d+)", cmd_line, int),
                              "cache_type_k": _f(r"--cache-type-k (\S+)", cmd_line, str),
                              "cache_type_v": _f(r"--cache-type-v (\S+)", cmd_line, str),
                              "flash_attn": _f(r"--flash-attn (\S+)", cmd_line, str),
                              "batch": _f(r" -b (\d+)", cmd_line, int), "ubatch": _f(r" -ub (\d+)", cmd_line, int)},
        "layers_offloaded": {"gpu": int(offload[-1][0]), "total": int(offload[-1][1])} if offload else None,
        "fit": ({"layers": int(fit[-1][0]), "layers_overflowing": int(fit[-1][1] or 0),
                 "vram_used_mib": int(fit[-1][2]), "vram_free_mib": int(fit[-1][3])} if fit else None),
        # one value per fit pass of the whole log: a first pass with the requested margin (+ a few MiB) and,
        # if the load then ran out of memory, a retry with a larger one (seen with fit_target_mib = 512)
        "fit_free_vram_targets_mib": [int(x) for x in re.findall(r"cannot meet free memory target of (\d+) MiB", text)],
        "model_buffer_gpu_mib": _f(r"CUDA0 model buffer size =\s+([\d.]+) MiB", seg),
        "model_buffer_cpu_mib": _f(r"CUDA_Host model buffer size =\s+([\d.]+) MiB", seg),
        "kv_cache": ({"mib": float(kv[-1][0]), "cells": int(kv[-1][1]), "layers": int(kv[-1][2]),
                      "k_type": kv[-1][3], "v_type": kv[-1][4]} if kv else None),
        "compute_buffer_gpu_mib": _f(r"sched_reserve:\s+CUDA0 compute buffer size =\s+([\d.]+) MiB", seg),
        "context": {"n_ctx": _f(r"llama_context: n_ctx\s+=\s+(\d+)", seg, int),
                    "flash_attn": _f(r"llama_context: flash_attn\s+=\s+(\w+)", seg, str)},
        "prompt_cache_limit_mib": _f(r"prompt cache is enabled, size limit: (\d+) MiB", seg, int),
        "context_checkpoints": ({"max": int(ckpt[-1][0]), "min_spacing": int(ckpt[-1][1])} if ckpt else None),
    }


def check_applied(requested: dict, facts: dict) -> list[str]:
    """Differences between what was asked for and what the log says the server did (empty = as requested)."""
    problems = []
    args, kv, ctx = facts.get("llama_server_args") or {}, facts.get("kv_cache"), facts.get("context") or {}
    if "num_ctx" in requested and ctx.get("n_ctx") not in (None, requested["num_ctx"]):
        problems.append(f"num_ctx {requested['num_ctx']} requested, llama.cpp context is {ctx['n_ctx']}")
    if "kv_cache_type" in requested and kv and (kv["k_type"], kv["v_type"]) != (requested["kv_cache_type"],) * 2:
        problems.append(f"kv_cache_type {requested['kv_cache_type']} requested, KV cache is K {kv['k_type']} / V {kv['v_type']}")
    if "flash_attention" in requested and ctx.get("flash_attn") is not None:
        on = ctx["flash_attn"] in ("enabled", "on")
        if on != requested["flash_attention"]:
            problems.append(f"flash_attention {requested['flash_attention']} requested, server reports {ctx['flash_attn']}")
    echoed = (facts.get("env") or {}).get("LLAMA_ARG_FIT_TARGET")
    if "fit_target_mib" in requested and echoed not in (None, str(requested["fit_target_mib"])):
        problems.append(f"fit_target_mib {requested['fit_target_mib']} requested, the server's environment has {echoed!r}")
    if "num_parallel" in requested and args.get("parallel") not in (None, requested["num_parallel"]):
        problems.append(f"num_parallel {requested['num_parallel']} requested, server runs {args['parallel']} slot(s)")
    return problems


# ---- talking to Ollama ------------------------------------------------------------------------------------------------

def _get(url: str, timeout: float = 5.0):
    try:
        r = httpx.get(url, timeout=timeout)
        return r.json() if r.status_code == 200 else None
    except (httpx.HTTPError, ValueError):
        return None


def release_vram(service_url: str = SERVICE_URL, wait_s: float = 60.0) -> list[str]:
    """Unload every model the (system) server holds so a private server can use the whole GPU."""
    names = [m["name"] for m in (_get(f"{service_url}/api/ps") or {}).get("models", [])]
    for n in names:
        httpx.post(f"{service_url}/api/generate", json={"model": n, "keep_alive": 0}, timeout=30)
    deadline = time.monotonic() + wait_s
    while names and time.monotonic() < deadline:
        if not (_get(f"{service_url}/api/ps") or {}).get("models"):
            break
        time.sleep(1)
    return names


def ensure_model(base_url: str, name: str, base: str, num_ctx: int) -> bool:
    """Make sure ``name`` exists with the given num_ctx (derived from ``base``); True if it had to be created.
    Done through the system server's API, which owns the model store."""
    shown = _get_post(f"{base_url}/api/show", {"model": name})
    if shown and re.search(rf"^num_ctx\s+{num_ctx}\s*$", shown.get("parameters", ""), re.M):
        return False
    r = httpx.post(f"{base_url}/api/create", json={"model": name, "from": base, "parameters": {"num_ctx": num_ctx},
                                                   "stream": False}, timeout=600)
    r.raise_for_status()
    return True


def _get_post(url: str, body: dict):
    try:
        r = httpx.post(url, json=body, timeout=30)
        return r.json() if r.status_code == 200 else None
    except (httpx.HTTPError, ValueError):
        return None


class ManagedServer:
    """A private ``ollama serve`` with the settings of one configuration; use as a context manager."""

    def __init__(self, server: dict, model: str, log_path: Path, host: str = PRIVATE_HOST,
                 models_dir: str = DEFAULT_MODELS_DIR, service_url: str = SERVICE_URL, ollama_bin: str = "ollama"):
        self.server, self.model, self.log_path = server, model, Path(log_path)
        self.host, self.models_dir, self.service_url, self.ollama_bin = host, models_dir, service_url, ollama_bin
        self.proc: subprocess.Popen | None = None
        self.facts: dict = {}
        self.created_model = False

    @property
    def url(self) -> str:
        return f"http://{self.host}"

    @property
    def v1(self) -> str:
        return f"{self.url}/v1"

    def start(self, ready_s: float = 90.0, load_timeout_s: float = 900.0) -> dict:
        if _get(f"{self.url}/api/tags", 1) is not None:
            raise RuntimeError(f"{self.host} is already in use")
        release_vram(self.service_url)
        if "num_ctx" in self.server:
            self.created_model = ensure_model(self.service_url, self.model, self.server.get("base_model", DEFAULT_BASE_MODEL),
                                              self.server["num_ctx"])
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        env = {**os.environ, **server_env(self.server, self.host, self.models_dir)}
        self.proc = subprocess.Popen([self.ollama_bin, "serve"], env=env, stdout=open(self.log_path, "wb"),
                                     stderr=subprocess.STDOUT, start_new_session=True)
        deadline = time.monotonic() + ready_s
        while _get(f"{self.url}/api/tags", 1) is None:
            if self.proc.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError(f"private ollama serve did not start (see {self.log_path})")
            time.sleep(0.5)
        # load the model now, so loading never lands in a measured trial, and read back what the server did
        httpx.post(f"{self.url}/api/generate", json={"model": self.model, "prompt": "hi", "stream": False,
                                                     "options": {"num_predict": 1}}, timeout=load_timeout_s).raise_for_status()
        self.facts = parse_server_log(self.log_path.read_text(errors="replace"))
        ps = (_get(f"{self.url}/api/ps") or {}).get("models") or [{}]
        self.facts["ps"] = {"size_vram_bytes": ps[0].get("size_vram"), "context_length": ps[0].get("context_length")}
        self.facts["requested"] = dict(self.server)
        self.facts["model"] = self.model
        self.facts["not_as_requested"] = check_applied(self.server, self.facts)
        return self.facts

    def stop(self, wait_s: float = 60.0) -> None:
        if self.proc and self.proc.poll() is None:
            os.killpg(self.proc.pid, signal.SIGINT)
            try:
                self.proc.wait(wait_s)
            except subprocess.TimeoutExpired:
                os.killpg(self.proc.pid, signal.SIGKILL)
                self.proc.wait()

    def __enter__(self) -> "ManagedServer":
        self.start()
        return self

    def __exit__(self, *exc) -> None:
        self.stop()


def record_server(platform_yaml: Path, config_id: str, facts: dict) -> None:
    """Add the facts of a configuration's server to ``platform.yaml`` under ``servers``."""
    data = yaml.safe_load(platform_yaml.read_text()) if platform_yaml.exists() else {}
    data = data or {}
    data.setdefault("servers", {})[config_id] = facts
    platform_yaml.write_text(yaml.safe_dump(data, sort_keys=False))


# ---- CLI: what does each configuration's server do? -----------------------------------------------------------------

def survey(config_paths: list[Path], out: Path) -> list[dict]:
    """Start the private server of every configuration in turn (no trials) and record what it did."""
    from harness import runner

    out.mkdir(parents=True, exist_ok=True)
    rows = []
    for p in config_paths:
        cfg = runner.load_config(p)
        if cfg.server is None:
            continue
        m = ManagedServer(cfg.server, cfg.model.split("/", 1)[1], out / f"{cfg.id}.log")
        t0 = time.monotonic()
        with m:
            rows.append({"config": cfg.id, "load_s": round(time.monotonic() - t0, 1), **m.facts})
    (out / "server_survey.json").write_text(json.dumps(rows, indent=2) + "\n")
    (out / "server_survey.md").write_text(render_survey(rows))
    return rows


def render_survey(rows: list[dict]) -> str:
    L = ["# Server survey", "",
         "Each configuration's private Ollama server was started once and read back (no trials). "
         "'Experts on CPU' = layers whose MoE experts overflow to system memory (llama.cpp fit).", "",
         "| Config | requested | n_ctx | KV (K/V) | KV MiB | flash attn | GPU layers | experts on CPU | model GPU MiB | model CPU MiB | VRAM used (fit) | size_vram | as requested |",
         "|---|---|---:|---|---:|---|---:|---:|---:|---:|---:|---:|---|"]
    for r in rows:
        kv, fit, lo = r.get("kv_cache") or {}, r.get("fit") or {}, r.get("layers_offloaded") or {}
        L.append(f"| {r['config']} | {json.dumps(r['requested'])} | {(r.get('context') or {}).get('n_ctx')} | "
                 f"{kv.get('k_type')}/{kv.get('v_type')} | {kv.get('mib')} | {(r.get('context') or {}).get('flash_attn')} | "
                 f"{lo.get('gpu')}/{lo.get('total')} | {fit.get('layers_overflowing')} | {r.get('model_buffer_gpu_mib')} | "
                 f"{r.get('model_buffer_cpu_mib')} | {fit.get('vram_used_mib')} | "
                 f"{round((r.get('ps') or {}).get('size_vram_bytes', 0) / 2**20)} | "
                 f"{'yes' if not r.get('not_as_requested') else '; '.join(r['not_as_requested'])} |")
    return "\n".join(L) + "\n"


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description="Private Ollama servers of run configurations")
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("survey", help="start each configuration's server once and record what it did")
    s.add_argument("--configs", type=Path, nargs="+", required=True)
    s.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    rows = survey(a.configs, a.out)
    print((a.out / "server_survey.md").read_text())
    return 0 if rows else 1


if __name__ == "__main__":
    raise SystemExit(main())
