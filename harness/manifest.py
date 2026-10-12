#!/usr/bin/env python3
"""
harness/manifest.py

Records the hardware/software manifest (platform.yaml) at the start of a run:
GPU, driver, CUDA, Ollama, OpenCode, model digests, Python. Every probe fails
soft (value "unavailable") so the manifest can be produced on a CI box too.

    python -m harness.manifest [-o platform.yaml]
"""

import argparse
import json
import platform
import re
import subprocess
import urllib.request
from datetime import UTC, datetime
from pathlib import Path

import yaml

UNAVAILABLE = "unavailable"


def _run(cmd: list[str]) -> str | None:
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL, timeout=10).strip()
    except (OSError, subprocess.SubprocessError):
        return None


def gpu_info() -> dict:
    out = _run(["nvidia-smi", "--query-gpu=name,driver_version,memory.total",
                "--format=csv,noheader,nounits"])
    info = {"name": UNAVAILABLE, "driver": UNAVAILABLE, "memory_mb": UNAVAILABLE,
            "cuda": UNAVAILABLE}
    if out:
        name, driver, mem = [x.strip() for x in out.splitlines()[0].split(",")]
        info.update(name=name, driver=driver, memory_mb=int(float(mem)))
    header = _run(["nvidia-smi"])
    if header and (m := re.search(r"CUDA Version:\s*([\d.]+)", header)):
        info["cuda"] = m.group(1)
    return info


def tool_version(cmd: list[str]) -> str:
    out = _run(cmd)
    if not out:
        return UNAVAILABLE
    m = re.search(r"\d+\.\d+(?:\.\d+)?", out)
    return m.group(0) if m else out.splitlines()[0]


def model_digests(base: str = "http://localhost:11434") -> dict[str, str]:
    try:
        with urllib.request.urlopen(f"{base}/api/tags", timeout=5) as r:
            tags = json.load(r)
        return {m["name"]: m.get("digest", UNAVAILABLE) for m in tags.get("models", [])}
    except (OSError, ValueError):
        return {}


def build_manifest() -> dict:
    return {
        "created_utc": datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "gpu": gpu_info(),
        "os": platform.platform(),
        "python": platform.python_version(),
        "ollama": tool_version(["ollama", "--version"]),
        "opencode": tool_version(["opencode", "--version"]),
        "models": model_digests(),
    }


def write_manifest(path: Path, manifest: dict | None = None) -> dict:
    manifest = manifest or build_manifest()
    path.write_text(yaml.safe_dump(manifest, sort_keys=False))
    return manifest


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", type=Path, default=Path("platform.yaml"))
    args = ap.parse_args()
    write_manifest(args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
