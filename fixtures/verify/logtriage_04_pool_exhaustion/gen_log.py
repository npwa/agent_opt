#!/usr/bin/env python3
"""Deterministically generate ~5 MB of service logs (app.log) with one buried root cause.

Usage: gen_log.py <outdir>

Ground truth (checked by the task's rubric, hidden from the agent):
  * first ERROR burst at 03:17:42 on host api-07
  * cause: DB connection pool exhaustion ("pool exhausted (max=20)") because the
    nightly job `reconcile-ledger` (started 03:15:03 by user svc-batch) holds 20 connections
  * unrelated noise: cache-miss WARNs, a red-herring TLS handshake error on api-02 at 02:41
"""
import random
import sys
from pathlib import Path

SEED = 20260118
TARGET_BYTES = 5_000_000
HOSTS = [f"api-{i:02d}" for i in range(1, 9)]
PATHS = ["/v1/orders", "/v1/users", "/v1/items", "/v1/search", "/v1/cart", "/healthz"]


def ts(sec: int) -> str:
    return f"2026-01-18T{sec // 3600:02d}:{sec % 3600 // 60:02d}:{sec % 60:02d}Z"


def hms(h, m, s):
    return h * 3600 + m * 60 + s


def main(outdir: str) -> None:
    rnd = random.Random(SEED)
    lines: list[tuple[int, int, str]] = []  # (second, tiebreak, text)
    n = 0
    while sum(len(x[2]) + 1 for x in lines) < TARGET_BYTES:
        for _ in range(2000):
            sec = rnd.randrange(hms(2, 0, 0), hms(4, 0, 0))
            host = rnd.choice(HOSTS)
            path = rnd.choice(PATHS)
            ms = int(rnd.lognormvariate(3.2, 0.5))
            roll = rnd.random()
            if roll < 0.04:
                text = f"{ts(sec)} WARN {host} cache miss key=sess:{rnd.randrange(10**6)} path={path}"
            elif roll < 0.045:
                text = f"{ts(sec)} WARN {host} slow request {path} took {ms * 20}ms"
            else:
                text = f"{ts(sec)} INFO {host} GET {path} 200 {ms}ms req={rnd.randrange(10**9):09d}"
            n += 1
            lines.append((sec, n, text))

    # red herring
    lines.append((hms(2, 41, 7), 0, f"{ts(hms(2, 41, 7))} ERROR api-02 TLS handshake failed: peer reset (client=10.4.2.9)"))
    # root cause chain
    lines.append((hms(3, 15, 3), 0, f"{ts(hms(3, 15, 3))} INFO batch-01 job reconcile-ledger started user=svc-batch db_conns=20"))
    for k in range(20):
        sec = hms(3, 15, 4) + k * 5
        lines.append((sec, 0, f"{ts(sec)} DEBUG batch-01 reconcile-ledger acquired connection {k + 1}/20 from pool=primary"))
    lines.append((hms(3, 17, 42), 0, f"{ts(hms(3, 17, 42))} ERROR api-07 db pool exhausted (max=20) waiting=14 path=/v1/orders"))
    for k in range(300):
        sec = hms(3, 17, 43) + k // 3
        host = rnd.choice(HOSTS)
        lines.append((sec, 1, f"{ts(sec)} ERROR {host} request failed 503 upstream=db pool exhausted (max=20) req={rnd.randrange(10**9):09d}"))
    lines.append((hms(3, 41, 30), 0, f"{ts(hms(3, 41, 30))} INFO batch-01 job reconcile-ledger finished rows=48210112 released_conns=20"))

    lines.sort(key=lambda x: (x[0], x[1]))
    out = Path(outdir)
    out.mkdir(parents=True, exist_ok=True)
    (out / "app.log").write_text("\n".join(t for _, _, t in lines) + "\n")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")
