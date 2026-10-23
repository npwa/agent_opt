"""
harness/nsys.py

Nsight Systems capture mode (F9): what does the GPU do, kernel by kernel, while an agent works,
and how does that line up with the proxy's view of the same run?

Pieces
* ``load_kernels`` / ``load_launch_calls``  read an ``nsys export --type sqlite`` file; timestamps
  are converted to epoch seconds (kernel `start` is ns since the capture started; the capture's
  UTC start time is in TARGET_INFO_SESSION_START_TIME) so they join with proxy/trace data.
* ``align``  compares kernel-active time with the proxy's prefill/decode windows and with NVML's
  utilisation, and reports the CPU-side launch gaps (GPU idle between consecutive kernels).
* ``capture``  (CLI) runs ONE trial with a private ``ollama serve`` wrapped in ``nsys profile``.

Findings that shaped the capture (see the docstring of ``NSYS_ARGS``):
* Ollama (0.32) serves models through a ``llama-server`` child process. ``--trace-fork-before-exec``
  is needed to follow it, and ``--cuda-flush-interval`` is essential: Ollama kills that child on
  unload, and without periodic flushes every kernel record of the process is lost.
* The system service runs as user ``ollama`` and cannot be wrapped without root, so the capture
  starts a second, private instance (127.0.0.1:11436, same read-only model store). The service
  must have no model loaded (VRAM), which is checked first.
* CUDA *hardware* tracing is unsupported on this GPU/driver combination; nsys falls back to a
  software-instrumented trace, so kernel durations are accurate to the tracer's resolution but
  carry some overhead - compare ratios, not absolute times, with an uninstrumented run.
"""

import argparse
import csv
import json
import os
import signal
import sqlite3
import subprocess
import sys
import time
import urllib.request
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from harness.decompose import merge
from harness.trace import percentile

NSYS_ARGS = ["--trace=cuda", "--trace-fork-before-exec=true", "--cuda-flush-interval=500",
             "--sample=none", "--cpuctxsw=none", "--force-overwrite=true"]
PRIVATE_HOST = "127.0.0.1:11436"
SERVICE_URL = "http://localhost:11434"
LAUNCH_API_PREFIXES = ("cudaLaunchKernel", "cuLaunchKernel", "cudaGraphLaunch", "cuGraphLaunch")


@dataclass(frozen=True)
class Kernel:
    start: float  # epoch seconds
    end: float
    name: str


# ---- reading the nsys export ------------------------------------------------------------------

def _session_epoch_s(conn: sqlite3.Connection) -> float:
    return conn.execute("select utcEpochNs from TARGET_INFO_SESSION_START_TIME").fetchone()[0] / 1e9


def _tables(conn: sqlite3.Connection) -> set[str]:
    return {r[0] for r in conn.execute("select name from sqlite_master where type='table'")}


GRAPH_NAME = "[CUDA graph launch]"


def load_kernels(sqlite_path: Path) -> list[Kernel]:
    """GPU activity as epoch-second intervals: individual kernels plus CUDA graph executions.

    llama.cpp runs the decode phase as CUDA graphs. nsys records those in a separate table
    (CUPTI_ACTIVITY_KIND_GRAPH_TRACE) and not as individual kernels, so reading only the kernel
    table would show an almost idle GPU during decode (verified on a real capture: 33k graph
    launches vs 87k kernels, and ~0 % kernel activity in decode windows before graphs were added).
    """
    conn = sqlite3.connect(str(sqlite_path))
    try:
        tables = _tables(conn)
        if "TARGET_INFO_SESSION_START_TIME" not in tables:
            return []
        t0 = _session_epoch_s(conn)
        names = dict(conn.execute("select id, value from StringIds")) if "StringIds" in tables else {}
        out = []
        if "CUPTI_ACTIVITY_KIND_KERNEL" in tables:
            for start, end, short, dem in conn.execute(
                    "select start, end, shortName, demangledName from CUPTI_ACTIVITY_KIND_KERNEL"):
                out.append(Kernel(t0 + start / 1e9, t0 + end / 1e9,
                                  str(names.get(short) or names.get(dem) or "?")[:120]))
        if "CUPTI_ACTIVITY_KIND_GRAPH_TRACE" in tables:
            for start, end in conn.execute("select start, end from CUPTI_ACTIVITY_KIND_GRAPH_TRACE"):
                out.append(Kernel(t0 + start / 1e9, t0 + end / 1e9, GRAPH_NAME))
        return sorted(out, key=lambda k: k.start)
    finally:
        conn.close()


def load_launch_calls(sqlite_path: Path) -> list[tuple[float, float]]:
    """(start, end) epoch seconds of CUDA kernel/graph launch API calls (CPU-side launch cost)."""
    conn = sqlite3.connect(str(sqlite_path))
    try:
        if not {"CUPTI_ACTIVITY_KIND_RUNTIME", "TARGET_INFO_SESSION_START_TIME"} <= _tables(conn):
            return []
        t0 = _session_epoch_s(conn)
        names = dict(conn.execute("select id, value from StringIds"))
        return [(t0 + s / 1e9, t0 + e / 1e9)
                for s, e, nid in conn.execute(
                    "select start, end, nameId from CUPTI_ACTIVITY_KIND_RUNTIME order by start")
                if str(names.get(nid, "")).startswith(LAUNCH_API_PREFIXES)]
    finally:
        conn.close()


# ---- analysis ---------------------------------------------------------------------------------------

def busy_intervals(kernels: list[Kernel]) -> list[tuple[float, float]]:
    return merge([(k.start, k.end) for k in kernels])


def active_s(intervals: list[tuple[float, float]], t0: float, t1: float) -> float:
    return sum(max(0.0, min(t1, b) - max(t0, a)) for a, b in intervals)


def launch_gap_stats(intervals: list[tuple[float, float]], t0: float, t1: float,
                     min_gap_s: float = 0.0) -> dict:
    """Idle gaps between consecutive kernel-active intervals that start inside [t0, t1)."""
    inside = [(a, b) for a, b in intervals if t0 <= a < t1]
    gaps = [max(0.0, b2 - a1) for (_, a1), (b2, _) in zip(inside, inside[1:], strict=False)
            if b2 - a1 > min_gap_s]
    ms = [g * 1000 for g in gaps]
    return {"n": len(gaps), "total_s": sum(gaps), "p50_ms": percentile(ms, 50),
            "p99_ms": percentile(ms, 99), "max_ms": max(ms) if ms else None}


def kernel_summary(kernels: list[Kernel], top: int = 25) -> list[dict]:
    agg: dict[str, list[float]] = defaultdict(list)
    for k in kernels:
        agg[k.name].append(k.end - k.start)
    total = sum(sum(v) for v in agg.values()) or 1.0
    rows = [{"kernel": n, "count": len(v), "total_ms": sum(v) * 1000, "avg_us": sum(v) / len(v) * 1e6,
             "pct_of_kernel_time": 100 * sum(v) / total} for n, v in agg.items()]
    return sorted(rows, key=lambda r: -r["total_ms"])[:top]


def align(trace: dict, kernels: list[Kernel], slice_s: float = 0.1) -> dict:
    """Kernel activity versus the proxy's prefill/decode windows (per step and overall)."""
    iv = busy_intervals(kernels)
    steps = []
    in_llm = 0.0
    decode_slices = decode_slices_with_kernels = 0
    llm_windows = []
    for st in trace["steps"]:
        ttft = st["ttft_s"] or 0.0
        pre = (st["t_start"], st["t_start"] + ttft)
        dec = (st["t_start"] + ttft, st["t_end"])
        llm_windows.append((st["t_start"], st["t_end"]))
        in_llm += active_s(iv, st["t_start"], st["t_end"])
        n_slices = max(1, int((dec[1] - dec[0]) / slice_s))
        hit = sum(active_s(iv, dec[0] + i * slice_s, dec[0] + (i + 1) * slice_s) > 0
                  for i in range(n_slices))
        decode_slices += n_slices
        decode_slices_with_kernels += hit
        steps.append({
            "step": st["step"],
            "kernel_busy_pct_prefill": _pct(active_s(iv, *pre), pre[1] - pre[0]),
            "kernel_busy_pct_decode": _pct(active_s(iv, *dec), dec[1] - dec[0]),
            "nvml_busy_pct_llm": st.get("gpu_busy_pct_llm"),
            "launch_gaps_decode": launch_gap_stats(iv, *dec),
        })
    total_kernel_s = sum(b - a for a, b in iv)
    s = trace["summary"]
    wall = (s["wall_start"], s["wall_end"])
    return {
        "trial_id": trace["trial_id"], "n_kernels": sum(k.name != GRAPH_NAME for k in kernels),
        "n_graph_launches": sum(k.name == GRAPH_NAME for k in kernels), "steps": steps,
        "summary": {
            "kernel_active_s_total": total_kernel_s,
            "kernel_active_s_in_run": active_s(iv, *wall),
            "kernel_time_in_llm_windows_pct": _pct(in_llm, active_s(iv, *wall)),
            "decode_slices_with_kernels_pct": _pct(decode_slices_with_kernels, decode_slices),
            "kernel_busy_pct_of_llm_time": _pct(in_llm, sum(b - a for a, b in llm_windows)),
            "nvml_busy_pct_llm": _mean([x["nvml_busy_pct_llm"] for x in steps]),
            "launch_gaps_run": launch_gap_stats(iv, *wall),
        },
    }


def _pct(a: float, b: float) -> float | None:
    return 100.0 * a / b if b else None


def _mean(xs: list) -> float | None:
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else None


def to_markdown(al: dict) -> str:
    s = al["summary"]
    f = lambda v, d=1: "n/a" if v is None else f"{v:.{d}f}"  # noqa: E731
    g = s["launch_gaps_run"]
    lines = [f"# Nsight Systems alignment: {al['trial_id']}", "",
             f"{al['n_kernels']} kernels and {al['n_graph_launches']} CUDA graph launches captured"
             f"{' (llama.cpp decodes with CUDA graphs)' if al['n_graph_launches'] else ''}; "
             f"GPU-active time inside the run: "
             f"{f(s['kernel_active_s_in_run'], 2)} s.", "",
             f"- GPU-active time inside proxy LLM windows: **{f(s['kernel_time_in_llm_windows_pct'])} %** "
             "(acceptance >= 90 %)",
             f"- Decode-window 100 ms slices with GPU activity: "
             f"**{f(s['decode_slices_with_kernels_pct'])} %**",
             f"- GPU-active share of LLM time: {f(s['kernel_busy_pct_of_llm_time'])} % vs "
             f"NVML utilisation {f(s['nvml_busy_pct_llm'])} %",
             f"- Idle gaps between kernel-active intervals: n={g['n']}, p50 {f(g['p50_ms'], 2)} ms, "
             f"p99 {f(g['p99_ms'], 2)} ms, max {f(g['max_ms'], 1)} ms, total {f(g['total_s'], 2)} s", "",
             "| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |",
             "|---:|---:|---:|---:|---:|---:|"]
    for st in al["steps"]:
        gp = st["launch_gaps_decode"]
        lines.append(f"| {st['step']} | {f(st['kernel_busy_pct_prefill'])} | "
                     f"{f(st['kernel_busy_pct_decode'])} | {f(st['nvml_busy_pct_llm'])} | "
                     f"{f(gp['p50_ms'], 2)} | {f(gp['p99_ms'], 2)} |")
    return "\n".join(lines) + "\n"


def analyze(sqlite_path: Path, trace: dict, out_dir: Path) -> dict:
    kernels = load_kernels(sqlite_path)
    out_dir.mkdir(parents=True, exist_ok=True)
    al = align(trace, kernels)
    al["launch_api_calls"] = len(load_launch_calls(sqlite_path))
    (out_dir / "nsys_alignment.json").write_text(json.dumps(al, indent=2))
    (out_dir / "nsys_alignment.md").write_text(to_markdown(al))
    rows = kernel_summary(kernels)
    with open(out_dir / "kernel_summary.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["kernel", "count", "total_ms", "avg_us", "pct_of_kernel_time"])
        w.writeheader()
        w.writerows(rows)
    return al


# ---- capture orchestration -----------------------------------------------------------------------

def _get(url: str, timeout: float = 5) -> dict | None:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as r:
            return json.load(r)
    except (OSError, ValueError):
        return None


def _post(url: str, body: dict, timeout: float = 300) -> bool:
    req = urllib.request.Request(url, json.dumps(body).encode(), {"content-type": "application/json"})
    try:
        urllib.request.urlopen(req, timeout=timeout).read()
        return True
    except OSError:
        return False


def check_preconditions() -> list[str]:
    problems = []
    if not subprocess.run(["which", "nsys"], capture_output=True).returncode == 0:
        problems.append("nsys not found on PATH")
    ps = _get(f"{SERVICE_URL}/api/ps")
    if ps and ps.get("models"):
        problems.append("the system Ollama service has a model loaded (VRAM): run `ollama stop <model>` "
                        "or wait for it to unload")
    for port in (11435, 11436):
        if _get(f"http://127.0.0.1:{port}/api/tags", 1) is not None:
            problems.append(f"port {port} is already in use")
    return problems


def _stop(proc: subprocess.Popen, sig=signal.SIGINT, wait_s: float = 60) -> None:
    if proc.poll() is None:
        os.killpg(proc.pid, sig)
        try:
            proc.wait(wait_s)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait()


def capture(task: str, config: Path, out: Path, scratch: Path, warmup: bool = True,
            models_dir: str = "/usr/share/ollama/.ollama/models") -> dict:
    from harness import runner, trace

    if problems := check_preconditions():
        raise RuntimeError("; ".join(problems))
    out.mkdir(parents=True, exist_ok=True)
    scratch.mkdir(parents=True, exist_ok=True)
    cfg = runner.load_config(config)
    model_name = cfg.model.split("/", 1)[1]
    rep = scratch / "capture"
    env = {**os.environ, "OLLAMA_HOST": PRIVATE_HOST, "OLLAMA_MODELS": models_dir}
    procs: list[subprocess.Popen] = []
    try:
        serve = subprocess.Popen(
            ["nsys", "profile", *NSYS_ARGS, "-o", str(rep), "ollama", "serve"], env=env,
            stdout=open(scratch / "ollama.log", "wb"), stderr=subprocess.STDOUT,
            start_new_session=True)
        procs.append(serve)
        for _ in range(60):
            if _get(f"http://{PRIVATE_HOST}/api/tags", 1) is not None:
                break
            time.sleep(1)
        else:
            raise RuntimeError("private ollama did not start")
        proxy = subprocess.Popen(
            [sys.executable, "-m", "uvicorn", "harness.proxy:app", "--host", "127.0.0.1",
             "--port", "11435"], cwd=runner.ROOT, start_new_session=True,
            env={**os.environ, "OLLAMA_BASE": f"http://{PRIVATE_HOST}",
                 "AGENT_OPT_DATA_DIR": str(out / "proxy")},
            stdout=open(scratch / "proxy.log", "wb"), stderr=subprocess.STDOUT)
        (out / "proxy").mkdir(exist_ok=True)
        procs.append(proxy)
        time.sleep(3)
        if warmup:  # load the model outside the measured trial (it stays in the kernel timeline)
            _post(f"http://{PRIVATE_HOST}/v1/chat/completions", {
                "model": model_name, "max_tokens": 1,
                "messages": [{"role": "user", "content": "hi"}]})
        subprocess.run(
            [sys.executable, "-m", "harness.runner", "run", "--configs", str(config), "--tasks", task,
             "--n", "1", "--ignore-task-trials", "--seed", "1", "--out", str(out / "run"), "--scratch", str(scratch / "work"),
             "--proxy-data", str(out / "proxy"), "--gpu-hz", "10"], cwd=runner.ROOT, check=True)
        # unload so the llama-server child exits, give CUPTI a moment, then stop the profiler
        _post(f"http://{PRIVATE_HOST}/api/generate", {"model": model_name, "keep_alive": 0}, 60)
        time.sleep(8)
    finally:
        for p in reversed(procs):
            _stop(p, wait_s=90 if p is procs[0] else 15)

    sqlite_path = scratch / "capture.sqlite"
    subprocess.run(["nsys", "export", "--type", "sqlite", "--force-overwrite", "true", "-o",
                    str(sqlite_path), str(rep) + ".nsys-rep"], check=True, capture_output=True)
    traces = trace.build_all(out / "run", out / "proxy" / "metrics.jsonl", out / "run" / "gpu_samples.csv")
    if not traces:
        raise RuntimeError("no finished trial to align")
    return analyze(sqlite_path, traces[0], out)


def main() -> int:
    ap = argparse.ArgumentParser(description="Nsight Systems capture and alignment (F9)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("capture", help="run one trial under nsys with a private Ollama instance")
    c.add_argument("--task", required=True)
    c.add_argument("--config", type=Path, default=Path("configs/baseline_32k.yaml"))
    c.add_argument("--out", type=Path, required=True)
    c.add_argument("--scratch", type=Path, required=True)
    c.add_argument("--no-warmup", action="store_true")
    a = sub.add_parser("analyze", help="analyse an existing nsys sqlite export against a trace.json")
    a.add_argument("--sqlite", type=Path, required=True)
    a.add_argument("--trace", type=Path, required=True)
    a.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    if args.cmd == "capture":
        al = capture(args.task, args.config, args.out, args.scratch, not args.no_warmup)
    else:
        al = analyze(args.sqlite, json.loads(args.trace.read_text()), args.out)
    print(to_markdown(al))
    return 0


if __name__ == "__main__":
    sys.exit(main())
