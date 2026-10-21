"""Private-vs-system server equivalence check (see EQUIVALENCE.md).

Run from the repository root: PYTHONPATH=. python results/v1/server_survey/equiv_check.py private|system <out dir>
"""
import json
import sys

import httpx

from harness import promptreplay as pr
from harness import server as srv

which = sys.argv[1]
body = {"model": "qwen3.6-32k", "stream": True, "stream_options": {"include_usage": True}, "max_tokens": 300,
        "messages": [{"role": "user", "content": "Write a long, detailed essay about the history of the printing press. Do not stop early."}]}
def decode_test(client, base):
    out = []
    for _ in range(3):
        r = pr.post_timed(client, f"{base}/chat/completions", body)
        out.append(r["completion_tokens"] / (r["total_s"] - r["ttft_s"]))
    return out
with httpx.Client(timeout=None) as c:
    if which == "private":
        m = srv.ManagedServer({"num_ctx": 32768}, "qwen3.6-32k", __import__("pathlib").Path(sys.argv[2]) / "equiv_private.log")
        with m:
            print("private decode tok/s", [round(x,1) for x in decode_test(c, m.v1)], flush=True)
            seq = pr.load_sequence("results/v1/replay_probe/proxy/requests/baseline_32k__repoqa_05_retry_policy__00")
            rows = pr.experiment(c, m.v1, seq, ["default"], repeats=2, log=print)
            json.dump(pr.summarise(rows), open(sys.argv[2] + "/equiv_private_replay.json", "w"), indent=1)
    else:
        base = "http://localhost:11434/v1"
        c.post("http://localhost:11434/api/generate", json={"model": "qwen3.6-32k", "prompt": "hi", "stream": False, "options": {"num_predict": 1}})
        print("system decode tok/s", [round(x,1) for x in decode_test(c, base)], flush=True)
