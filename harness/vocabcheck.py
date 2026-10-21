"""
harness/vocabcheck.py

Which models could serve as the draft model for speculative decoding of the target model (plan item P9)?
llama.cpp only accepts a draft whose vocabulary matches the target's (harness/ggufmeta.py mirrors its rule),
and the lesson from `TensorRT_plugin` Phase 6 applies: *a shared tokenizer family is not enough*, so every
candidate is checked token by token before any timing is trusted.

Candidates are the models in the local Ollama store and, optionally, models of the Ollama registry that are
not installed: for those only the first megabytes of the GGUF file (the metadata with the vocabulary) are
fetched with an HTTP range request, never the weights. All GGUF parsing runs in an isolated interpreter
(``python -I``), because downloaded bytes are untrusted.

    python -m harness.vocabcheck --target qwen3.6-32k --remote qwen3.5:0.8b qwen3.5:2b --out results/v1/spec_decode
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

import httpx

ROOT = Path(__file__).resolve().parent.parent
GGUFMETA = ROOT / "harness" / "ggufmeta.py"
STORE = Path("/usr/share/ollama/.ollama/models")
REGISTRY = "https://registry.ollama.ai/v2/library"
HEADER_BYTES = 64 * 1024 * 1024
MANIFEST_ACCEPT = "application/vnd.docker.distribution.manifest.v2+json"


def split_name(name: str) -> tuple[str, str]:
    n, _, tag = name.partition(":")
    return n, tag or "latest"


def local_blob(name: str, store: Path = STORE) -> tuple[Path, int] | None:
    """(GGUF blob path, size) of an installed model, from the store's manifests."""
    n, tag = split_name(name)
    mf = store / "manifests/registry.ollama.ai/library" / n / tag
    if not mf.is_file():
        return None
    for layer in json.loads(mf.read_text()).get("layers", []):
        if layer.get("mediaType", "").endswith("image.model"):
            blob = store / "blobs" / layer["digest"].replace(":", "-")
            return (blob, layer["size"]) if blob.is_file() else None
    return None


def local_models(store: Path = STORE) -> list[str]:
    root = store / "manifests/registry.ollama.ai/library"
    return sorted(f"{d.name}:{t.name}" for d in root.iterdir() if d.is_dir() for t in d.iterdir() if t.is_file())


def fetch_remote_header(name: str, scratch: Path, client: httpx.Client | None = None,
                        header_bytes: int = HEADER_BYTES) -> tuple[Path, int]:
    """Download the first ``header_bytes`` of a registry model's GGUF (an HTTP range request); (path, full size)."""
    n, tag = split_name(name)
    own = client is None
    client = client or httpx.Client(follow_redirects=True, timeout=120)
    try:
        r = client.get(f"{REGISTRY}/{n}/manifests/{tag}", headers={"Accept": MANIFEST_ACCEPT})
        r.raise_for_status()
        layer = next(ly for ly in r.json()["layers"] if ly["mediaType"].endswith("image.model"))
        scratch.mkdir(parents=True, exist_ok=True)
        dest = scratch / (name.replace(":", "_").replace("/", "_") + ".gguf.part")
        with client.stream("GET", f"{REGISTRY}/{n}/blobs/{layer['digest']}",
                           headers={"Range": f"bytes=0-{header_bytes - 1}"}) as resp:
            resp.raise_for_status()
            with open(dest, "wb") as f:
                for chunk in resp.iter_bytes(1 << 20):
                    f.write(chunk)
        return dest, layer["size"]
    finally:
        if own:
            client.close()


def ggufmeta(*args: str) -> dict:
    """Run harness/ggufmeta.py in an isolated interpreter and parse its JSON output."""
    p = subprocess.run([sys.executable, "-I", str(GGUFMETA), *args], capture_output=True, text=True, timeout=300)
    try:
        return json.loads(p.stdout)
    except json.JSONDecodeError:
        return {"error": (p.stderr or p.stdout or "no output").strip()[-300:]}


def check(target: str, locals_: list[str], remotes: list[str], scratch: Path, store: Path = STORE) -> dict:
    t = local_blob(target, store)
    if t is None:
        raise SystemExit(f"target {target} is not installed")
    summary = ggufmeta("summary", str(t[0]))
    rows = []
    sources = [(n, "installed") for n in locals_] + [(n, "registry (header only)") for n in remotes]
    for name, origin in sources:
        try:
            if origin == "installed":
                found = local_blob(name, store)
                if found is None:
                    rows.append({"model": name, "origin": origin, "error": "not found in the store"})
                    continue
                path, size = found
            else:
                path, size = fetch_remote_header(name, scratch)
            res = ggufmeta("compare", str(t[0]), str(path))
            s = ggufmeta("summary", str(path))
            rows.append({"model": name, "origin": origin, "size_gb": round(size / 1e9, 2), **{
                "compatible": res.get("compatible"), "reasons": res.get("reasons", [res.get("error")] if "error" in res else []),
                "draft_tokens": res.get("draft_tokens"), "identical_token_list": res.get("identical_token_list"),
                "architecture": s.get("architecture"), "tokenizer_model": (s.get("tokenizer") or {}).get("model"),
                "tokenizer_pre": (s.get("tokenizer") or {}).get("pre"), "tokens_sha": s.get("tokens_sha")}})
        except (httpx.HTTPError, OSError, StopIteration, KeyError, ValueError) as e:
            rows.append({"model": name, "origin": origin, "error": f"{type(e).__name__}: {e}"})
    return {"target": {"model": target, "size_gb": round(t[1] / 1e9, 2), **{k: summary.get(k) for k in (
        "architecture", "n_tokens", "tokens_sha", "tokenizer", "mtp_tensors", "block_count", "expert_count")}},
        "candidates": rows}


def render(res: dict) -> str:
    t = res["target"]
    tok = t["tokenizer"] or {}
    L = ["# Draft-model vocabulary check", "",
         f"Target **{t['model']}** ({t['size_gb']} GB): architecture `{t['architecture']}`, tokenizer `{tok.get('model')}` / "
         f"`{tok.get('pre')}`, {t['n_tokens']} tokens (sha `{t['tokens_sha']}`), eos {tok.get('eos_token_id')}, "
         f"{len(t['mtp_tensors'] or [])}+ built-in multi-token-prediction tensors (`{(t['mtp_tensors'] or [''])[0]}`...).", "",
         "Rule (llama.cpp `common_speculative_are_compatible`): same vocabulary type, same add_bos/add_eos (and ids), vocabulary "
         "size within 128, identical token text from id 5 on. 'Same tokens' = the whole token list is identical.", "",
         "| Candidate | Origin | Size GB | Architecture | Tokenizer | Tokens | Compatible | Why not |", "|---|---|---:|---|---|---:|---|---|"]
    for r in sorted(res["candidates"], key=lambda r: (not r.get("compatible"), r["model"])):
        if "error" in r:
            L.append(f"| {r['model']} | {r['origin']} | | | | | error | {r['error']} |")
            continue
        why = "; ".join(x for x in r["reasons"] if x)[:200]
        same = " (same list)" if r.get("identical_token_list") else ""
        L.append(f"| {r['model']} | {r['origin']} | {r['size_gb']} | {r['architecture']} | {r['tokenizer_model']}/{r['tokenizer_pre']} | "
                 f"{r['draft_tokens']} | {'**yes**' + same if r['compatible'] else 'no'} | {why} |")
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Vocabulary compatibility of candidate draft models with a target")
    ap.add_argument("--target", required=True, help="installed Ollama model, e.g. qwen3.6-32k")
    ap.add_argument("--local", nargs="*", help="installed models to check (default with --local-all: every installed model)")
    ap.add_argument("--local-all", action="store_true")
    ap.add_argument("--remote", nargs="*", default=[], help="registry models whose GGUF header is fetched (range request)")
    ap.add_argument("--scratch", type=Path, default=Path("/tmp/agent_opt_vocab"))
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    locals_ = local_models() if a.local_all else (a.local or [])
    res = check(a.target, locals_, a.remote, a.scratch)
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "vocab_check.json").write_text(json.dumps(res, indent=2) + "\n")
    (a.out / "vocab_check.md").write_text(render(res))
    print(render(res))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
