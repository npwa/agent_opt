"""P12: what does the server's prompt cache reuse? Prime with a recorded first call, then send an edited copy and time it.

A fast second call (< 3 s) means the edited part comes after the cached prefix or does not matter; a slow one (~ cold) means the
edit sits early in the rendered prompt. Edits: identical, other seed, working directory changed, text appended to the last
user message, end of the system message changed, first / last tool description changed.

    python scripts/p12_prefix_probe.py --requests <trial dir>/a1_001.json ... --out results/v1/prefix_probe
"""
import argparse
import json
import random
from pathlib import Path

import httpx

from harness import promptreplay as pr

BASE = "http://localhost:11434/v1"


def edits(body: dict) -> dict:
    def sys_idx(b):
        return pr._system_indexes(b)[0]

    def tail(b):
        b["messages"][-1]["content"] = str(b["messages"][-1]["content"]) + "\nExtra tail text."
        return b

    def sys_end(b):
        b["messages"][sys_idx(b)]["content"] += "\nExtra closing line."
        return b

    def tool_first(b):
        b["tools"][0]["function"]["description"] += " (edited)"
        return b

    def tool_last(b):
        b["tools"][-1]["function"]["description"] += " (edited)"
        return b

    def seed(b):
        b["seed"] = (b.get("seed") or 0) + 1
        return b

    def clone(f):
        return lambda b: f(json.loads(json.dumps(b)))
    return {"identical": lambda b: b, "other_seed": clone(seed), "workdir_changed": lambda b: pr.perturb(b, "probe_dir"),
            "user_tail": clone(tail), "system_end": clone(sys_end), "first_tool_edit": clone(tool_first), "last_tool_edit": clone(tool_last)}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--requests", type=Path, nargs="+", required=True)
    ap.add_argument("--repeats", type=int, default=2)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    rng, rows = random.Random(1), []
    with httpx.Client(timeout=1200) as client:
        for path in a.requests:
            body = json.loads(path.read_text())
            task = path.parent.name
            for name, edit in edits(body).items():
                for rep in range(a.repeats):
                    nonce = f"{rng.getrandbits(48):012x}"
                    first = pr.add_nonce(body, nonce)
                    prime = pr.post_timed(client, f"{BASE}/chat/completions", pr.prepare(first, 1, None))
                    second = pr.post_timed(client, f"{BASE}/chat/completions", pr.prepare(edit(first), 1, None))
                    rows.append({"task": task, "edit": name, "rep": rep, "prime_s": round(prime["ttft_s"], 2),
                                 "second_s": round(second["ttft_s"], 2), "tokens": second["prompt_tokens"]})
                    print(rows[-1], flush=True)
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "prefix_probe.json").write_text(json.dumps(rows, indent=1) + "\n")
    L = ["| Task | Edit | cold prime (s) | edited second call (s) | prompt tokens |", "|---|---|---:|---:|---:|"]
    for r in rows:
        L.append(f"| {r['task']} | {r['edit']} | {r['prime_s']} | {r['second_s']} | {r['tokens']} |")
    (a.out / "prefix_probe.md").write_text("\n".join(L) + "\n")


if __name__ == "__main__":
    main()
