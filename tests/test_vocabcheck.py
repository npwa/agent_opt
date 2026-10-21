import json
import struct
import sys
from pathlib import Path

import httpx
import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "harness"))
import ggufmeta as gm  # noqa: E402

from harness import vocabcheck as vc  # noqa: E402


def _s(x: str) -> bytes:
    b = x.encode()
    return struct.pack("<Q", len(b)) + b


def _val(v) -> bytes:
    if isinstance(v, bool):
        return struct.pack("<I?", 7, v)
    if isinstance(v, int):
        return struct.pack("<II", 4, v)
    if isinstance(v, str):
        return struct.pack("<I", 8) + _s(v)
    if isinstance(v, list):  # list of str or int
        et = 8 if v and isinstance(v[0], str) else 4
        body = b"".join(_s(x) if et == 8 else struct.pack("<I", x) for x in v)
        return struct.pack("<IIQ", 9, et, len(v)) + body
    raise TypeError(v)


def make_gguf(path: Path, kv: dict, tensors=()) -> Path:
    out = b"GGUF" + struct.pack("<IQQ", 3, len(tensors), len(kv))
    for k, v in kv.items():
        out += _s(k) + _val(v)
    for name in tensors:
        out += _s(name) + struct.pack("<I", 1) + struct.pack("<Q", 8) + struct.pack("<I", 0) + struct.pack("<Q", 0)
    path.write_bytes(out + b"\0" * 64)
    return path


def vocab(n=300, model="gpt2", **extra):
    toks = [f"t{i}" for i in range(n)]
    return {"general.architecture": "qwen35moe", "tokenizer.ggml.model": model, "tokenizer.ggml.tokens": toks,
            "tokenizer.ggml.token_type": [1] * n, "tokenizer.ggml.eos_token_id": 7, **extra}


def test_roundtrip_summary_and_mtp_tensor_detection(tmp_path):
    p = make_gguf(tmp_path / "a.gguf", vocab(extra_key := 50) | {"qwen35moe.block_count": 40},
                  tensors=["blk.0.attn_q.weight", "mtp.layers.0.attn_q.weight", "blk.1.nextn.eh_proj.weight"])
    m = gm.read_metadata(str(p))
    assert m["version"] == 3 and m["tensor_count"] == 3 and m["tensors"][1] == "mtp.layers.0.attn_q.weight"
    s = gm.summarise(m)
    assert s["n_tokens"] == extra_key and s["block_count"] == 40 and s["tokenizer"]["model"] == "gpt2"
    assert s["mtp_tensors"] == ["mtp.layers.0.attn_q.weight", "blk.1.nextn.eh_proj.weight"]
    assert s["tokens_sha"] == gm.summarise(gm.read_metadata(str(p)))["tokens_sha"]  # deterministic


def test_bad_files_are_rejected_or_tolerated(tmp_path):
    (tmp_path / "x").write_bytes(b"NOPE" + b"\0" * 40)
    with pytest.raises(gm.GGUFError, match="not a GGUF"):
        gm.read_metadata(str(tmp_path / "x"))
    good = make_gguf(tmp_path / "g.gguf", vocab(20), tensors=["a", "b"])
    data = good.read_bytes()
    (tmp_path / "cut_kv").write_bytes(data[:40])  # ends inside the key/value block
    with pytest.raises(gm.GGUFError, match="truncated|bad string"):
        gm.read_metadata(str(tmp_path / "cut_kv"))
    (tmp_path / "cut_tensors").write_bytes(data[:len(data) - 64 - 10])  # inside the last tensor info: tolerated
    assert gm.read_metadata(str(tmp_path / "cut_tensors"))["kv"]["tokenizer.ggml.model"] == "gpt2"


def meta(kv):
    return {"kv": kv, "tensors": [], "tensor_count": 0, "version": 3}


def test_compatibility_rule_mirrors_llama_cpp():
    base = vocab(300)
    assert gm.compatible(meta(base), meta(dict(base)))["compatible"]
    # size difference: 128 allowed, 129 not
    assert gm.compatible(meta(base), meta(vocab(300 - 128)))["compatible"] is True
    assert "size differs" in gm.compatible(meta(base), meta(vocab(300 - 129)))["reasons"][0]
    # token text differs from id 5 on, but ids 0-4 are not compared
    d = vocab(300)
    d["tokenizer.ggml.tokens"][3] = "other"
    assert gm.compatible(meta(base), meta(d))["compatible"]
    d["tokenizer.ggml.tokens"][5] = "other"
    r = gm.compatible(meta(base), meta(d))
    assert not r["compatible"] and "from id 5" in r["reasons"][0]
    assert not gm.compatible(meta(base), meta(vocab(300, model="llama")))["compatible"]


def test_missing_add_bos_eos_keys_use_the_vocabulary_type_defaults():
    base = vocab(300)  # BPE: add_bos / add_eos default to False
    explicit = vocab(300, **{"tokenizer.ggml.add_bos_token": False, "tokenizer.ggml.add_eos_token": False})
    assert gm.add_flags(base) == (False, False) == gm.add_flags(explicit)
    assert gm.compatible(meta(base), meta(explicit))["compatible"]
    assert gm.add_flags({"tokenizer.ggml.model": "llama"}) == (True, False)  # SPM
    assert gm.add_flags({"tokenizer.ggml.model": "bert"}) == (True, False)
    assert gm.add_flags({"tokenizer.ggml.model": "t5"}) == (False, True)
    on = vocab(300, **{"tokenizer.ggml.add_bos_token": True, "tokenizer.ggml.bos_token_id": 1})
    assert "add_bos_token" in gm.compatible(meta(base), meta(on))["reasons"][0]
    other_id = vocab(300, **{"tokenizer.ggml.add_bos_token": True, "tokenizer.ggml.bos_token_id": 2})
    assert not gm.compatible(meta(on), meta(other_id))["compatible"]  # both add bos, ids differ


def fake_store(tmp_path, models: dict[str, Path]) -> Path:
    store = tmp_path / "store"
    for name, blob in models.items():
        n, _, tag = name.partition(":")
        digest = "sha256:" + n + tag
        (store / "blobs").mkdir(parents=True, exist_ok=True)
        dest = store / "blobs" / digest.replace(":", "-")
        dest.write_bytes(blob.read_bytes())
        mf = store / "manifests/registry.ollama.ai/library" / n / (tag or "latest")
        mf.parent.mkdir(parents=True, exist_ok=True)
        mf.write_text(json.dumps({"layers": [{"mediaType": "application/vnd.ollama.image.model", "digest": digest,
                                              "size": dest.stat().st_size},
                                             {"mediaType": "application/vnd.ollama.image.params", "digest": "sha256:x", "size": 1}]}))
    return store


def test_local_models_and_check_end_to_end(tmp_path):
    t = make_gguf(tmp_path / "t.gguf", vocab(300), ["mtp.fc.weight"])
    ok = make_gguf(tmp_path / "ok.gguf", vocab(310))
    bad = make_gguf(tmp_path / "bad.gguf", vocab(300, model="llama"))
    store = fake_store(tmp_path, {"target:latest": t, "good:1b": ok, "other:7b": bad})
    assert vc.local_models(store) == ["good:1b", "other:7b", "target:latest"]
    assert vc.local_blob("nope:1", store) is None and vc.local_blob("good:1b", store)[0].is_file()
    res = vc.check("target", ["good:1b", "other:7b", "missing:1"], [], tmp_path / "scratch", store)
    by = {r["model"]: r for r in res["candidates"]}
    assert by["good:1b"]["compatible"] is True and by["other:7b"]["compatible"] is False
    assert "error" in by["missing:1"] and res["target"]["mtp_tensors"] == ["mtp.fc.weight"]
    md = vc.render(res)
    assert "| good:1b |" in md and md.index("good:1b") < md.index("other:7b")  # compatible ones first


def test_fetch_remote_header_asks_for_a_byte_range_only(tmp_path):
    blob = make_gguf(tmp_path / "r.gguf", vocab(50)).read_bytes()
    seen = []

    def handler(request: httpx.Request) -> httpx.Response:
        seen.append((request.url.path, request.headers.get("range")))
        if "/manifests/" in request.url.path:
            return httpx.Response(200, json={"layers": [{"mediaType": "application/vnd.ollama.image.model",
                                                         "digest": "sha256:abc", "size": 5_000_000_000}]})
        return httpx.Response(206, content=blob)

    client = httpx.Client(transport=httpx.MockTransport(handler), follow_redirects=True)
    path, size = vc.fetch_remote_header("qwen3.5:0.8b", tmp_path / "scratch", client, header_bytes=1000)
    assert size == 5_000_000_000 and path.read_bytes() == blob
    assert seen[0][0].endswith("/qwen3.5/manifests/0.8b") and seen[1] == ("/v2/library/qwen3.5/blobs/sha256:abc", "bytes=0-999")
    assert gm.read_metadata(str(path))["kv"]["tokenizer.ggml.model"] == "gpt2"


def test_ggufmeta_runs_isolated_and_reports_errors_as_json(tmp_path):
    (tmp_path / "x").write_bytes(b"NOPE" * 10)
    assert "not a GGUF" in vc.ggufmeta("summary", str(tmp_path / "x"))["error"]
    ok = make_gguf(tmp_path / "ok.gguf", vocab(30))
    assert vc.ggufmeta("compare", str(ok), str(ok))["compatible"] is True
