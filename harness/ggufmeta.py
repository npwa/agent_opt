"""
harness/ggufmeta.py

Stdlib-only reader of GGUF *metadata* (the key/value block and the tensor names, never the weights) and the
vocabulary-compatibility rule llama.cpp applies before it allows speculative decoding.

It is a standalone script on purpose: files that were downloaded (even just their first megabytes) are
untrusted input, so ``harness/vocabcheck.py`` runs it as ``python -I harness/ggufmeta.py ...`` (isolated
mode: no user site, no PYTHONPATH, no current directory on ``sys.path``).

    python -I harness/ggufmeta.py summary FILE
    python -I harness/ggufmeta.py compare TARGET.gguf DRAFT.gguf

The compatibility rule mirrors ``common_speculative_are_compatible`` in llama.cpp/common/speculative.cpp
(commit b2f036c0f): same vocabulary type, same add_bos/add_eos flags (and ids when they are added), a
vocabulary size difference of at most SPEC_VOCAB_MAX_SIZE_DIFFERENCE = 128, and identical token text for
every id from SPEC_VOCAB_CHECK_START_TOKEN_ID = 5 up to the smaller vocabulary.
"""

import hashlib
import json
import mmap
import struct
import sys

SPEC_VOCAB_MAX_SIZE_DIFFERENCE = 128
SPEC_VOCAB_CHECK_START_TOKEN_ID = 5
MAX_STRING = 1 << 24
MAX_ARRAY = 1 << 22
SCALARS = {0: "<B", 1: "<b", 2: "<H", 3: "<h", 4: "<I", 5: "<i", 6: "<f", 7: "<?", 10: "<Q", 11: "<q", 12: "<d"}


class GGUFError(ValueError):
    pass


class _Reader:
    def __init__(self, buf):
        self.buf, self.pos = buf, 0

    def take(self, fmt: str):
        size = struct.calcsize(fmt)
        if self.pos + size > len(self.buf):
            raise GGUFError("file ends inside the metadata (truncated download?)")
        v = struct.unpack_from(fmt, self.buf, self.pos)[0]
        self.pos += size
        return v

    def string(self) -> str:
        n = self.take("<Q")
        if n > MAX_STRING or self.pos + n > len(self.buf):
            raise GGUFError("bad string length")
        s = bytes(self.buf[self.pos:self.pos + n]).decode("utf-8", errors="replace")
        self.pos += n
        return s

    def value(self, t: int):
        if t in SCALARS:
            return self.take(SCALARS[t])
        if t == 8:
            return self.string()
        if t == 9:
            et, n = self.take("<I"), self.take("<Q")
            if n > MAX_ARRAY:
                raise GGUFError("array too large")
            return [self.value(et) for _ in range(n)]
        raise GGUFError(f"unknown value type {t}")


def read_metadata(path: str) -> dict:
    """{"version", "tensor_count", "kv": {key: value}, "tensors": [names]} from a (possibly truncated) GGUF file."""
    with open(path, "rb") as f:
        buf = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        try:
            if buf[:4] != b"GGUF":
                raise GGUFError("not a GGUF file")
            r = _Reader(buf)
            r.pos = 4
            version, n_tensors, n_kv = r.take("<I"), r.take("<Q"), r.take("<Q")
            if version not in (2, 3):
                raise GGUFError(f"unsupported GGUF version {version}")
            kv = {}
            for _ in range(n_kv):
                key = r.string()
                kv[key] = r.value(r.take("<I"))
            names = []
            try:
                for _ in range(n_tensors):
                    names.append(r.string())
                    for _ in range(r.take("<I")):
                        r.take("<Q")
                    r.take("<I")
                    r.take("<Q")
            except GGUFError:  # tensor infos beyond a truncated download: the key/value block is what matters
                pass
            return {"version": version, "tensor_count": n_tensors, "kv": kv, "tensors": names}
        finally:
            buf.close()


def _h(items) -> str:
    return hashlib.sha256("\x00".join(map(str, items)).encode("utf-8", errors="replace")).hexdigest()[:16]


def summarise(meta: dict) -> dict:
    kv, arch = meta["kv"], meta["kv"].get("general.architecture", "")
    tokens = kv.get("tokenizer.ggml.tokens") or []
    types = kv.get("tokenizer.ggml.token_type") or []
    merges = kv.get("tokenizer.ggml.merges") or []
    get = lambda k: kv.get(f"{arch}.{k}")  # noqa: E731
    return {
        "architecture": arch, "name": kv.get("general.name"), "size_label": kv.get("general.size_label"),
        "file_type": kv.get("general.file_type"), "tensor_count": meta["tensor_count"],
        "tokenizer": {k.removeprefix("tokenizer.ggml."): kv[k] for k in sorted(kv)
                      if k.startswith("tokenizer.ggml.") and not isinstance(kv[k], list)},
        "n_tokens": len(tokens), "tokens_sha": _h(tokens), "token_types_sha": _h(types), "n_merges": len(merges),
        "merges_sha": _h(merges), "first_tokens": tokens[:6], "last_tokens": tokens[-4:],
        "chat_template_sha": _h([kv["tokenizer.chat_template"]]) if "tokenizer.chat_template" in kv else None,
        "context_length": get("context_length"), "block_count": get("block_count"),
        "expert_count": get("expert_count"), "nextn_predict_layers": get("nextn_predict_layers"),
        "mtp_tensors": [n for n in meta["tensors"] if "nextn" in n.lower() or "mtp" in n.lower()][:12],
    }


def add_flags(kv: dict) -> tuple[bool, bool]:
    """(add_bos, add_eos) as llama.cpp resolves them: the GGUF key when present, else the default of the
    vocabulary type (SPM 'llama' and WPM 'bert': bos on; UGM 't5': eos on; BPE 'gpt2' and others: both off)."""
    model = kv.get("tokenizer.ggml.model")
    bos, eos = {"llama": (True, False), "bert": (True, False), "t5": (False, True)}.get(model, (False, False))
    return (bool(kv.get("tokenizer.ggml.add_bos_token", bos)), bool(kv.get("tokenizer.ggml.add_eos_token", eos)))


def compatible(target: dict, draft: dict) -> dict:
    """llama.cpp's speculative-decoding vocabulary check between two metadata dicts (see module docstring)."""
    kt, kd = target["kv"], draft["kv"]
    tt, td = kt.get("tokenizer.ggml.tokens") or [], kd.get("tokenizer.ggml.tokens") or []
    reasons = []
    mt, md = kt.get("tokenizer.ggml.model"), kd.get("tokenizer.ggml.model")
    if mt != md:
        reasons.append(f"vocabulary type differs: target {mt!r}, draft {md!r}")
    ft, fd = add_flags(kt), add_flags(kd)
    for flag, idkey, a, b in (("add_bos_token", "bos_token_id", ft[0], fd[0]), ("add_eos_token", "eos_token_id", ft[1], fd[1])):
        ia, ib = kt.get(f"tokenizer.ggml.{idkey}"), kd.get(f"tokenizer.ggml.{idkey}")
        if a != b or (a and ia != ib):
            reasons.append(f"{flag} differs: target {a} (id {ia}), draft {b} (id {ib})")
    diff = abs(len(tt) - len(td))
    if diff > SPEC_VOCAB_MAX_SIZE_DIFFERENCE:
        reasons.append(f"vocabulary size differs by {diff} (> {SPEC_VOCAB_MAX_SIZE_DIFFERENCE}): target {len(tt)}, draft {len(td)}")
    first_diff, n_diff = None, 0
    for i in range(SPEC_VOCAB_CHECK_START_TOKEN_ID, min(len(tt), len(td))):
        if tt[i] != td[i]:
            n_diff += 1
            if first_diff is None:
                first_diff = i
    if n_diff:
        reasons.append(f"{n_diff} token texts differ from id {first_diff} (target {tt[first_diff]!r}, draft {td[first_diff]!r})")
    return {"compatible": not reasons, "reasons": reasons, "target_tokens": len(tt), "draft_tokens": len(td),
            "identical_token_list": tt == td}


def main(argv: list[str]) -> int:
    if len(argv) >= 2 and argv[0] == "summary":
        print(json.dumps(summarise(read_metadata(argv[1])), indent=1, default=str))
        return 0
    if len(argv) == 3 and argv[0] == "compare":
        res = compatible(read_metadata(argv[1]), read_metadata(argv[2]))
        print(json.dumps(res, indent=1, default=str))
        return 0 if res["compatible"] else 1
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except GGUFError as e:
        print(json.dumps({"error": str(e)}))
        raise SystemExit(3) from e
