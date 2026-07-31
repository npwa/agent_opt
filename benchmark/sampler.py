#!/usr/bin/env python3
"""
sampler.py

Background system resource sampler. Run this in a separate terminal for the
duration of your whole benchmark session (start it before task 1, Ctrl+C
after the last task). It writes one row per second to samples.csv with
wall-clock time, GPU utilization/memory (via nvidia-smi), and CPU/RAM use
(via psutil), so metrics_proxy.py's per-request timestamps can be joined
against it afterward.

Requires: pip install psutil
Also requires `nvidia-smi` on PATH (standard with any NVIDIA driver install).
Run:      python sampler.py
"""

import csv
import subprocess
import time
from pathlib import Path

import psutil

OUT_PATH = Path(__file__).parent / "samples.csv"
INTERVAL_S = 1.0


def read_gpu():
    try:
        out = subprocess.check_output(
            [
                "nvidia-smi",
                "--query-gpu=utilization.gpu,utilization.memory,memory.used,memory.total",
                "--format=csv,noheader,nounits",
            ],
            text=True,
        ).strip()
        gpu_util, mem_util, mem_used, mem_total = [x.strip() for x in out.split(",")]
        return float(gpu_util), float(mem_util), float(mem_used), float(mem_total)
    except Exception:
        return None, None, None, None


def main():
    print(f"Sampling every {INTERVAL_S}s -> {OUT_PATH}. Ctrl+C to stop.")
    new_file = not OUT_PATH.exists()
    with open(OUT_PATH, "a", newline="") as f:
        writer = csv.writer(f)
        if new_file:
            writer.writerow(
                ["wall_time", "gpu_util_pct", "gpu_mem_util_pct",
                 "gpu_mem_used_mb", "gpu_mem_total_mb", "cpu_pct", "ram_pct"]
            )
        try:
            while True:
                wall_time = time.time()
                gpu_util, gpu_mem_util, gpu_mem_used, gpu_mem_total = read_gpu()
                cpu_pct = psutil.cpu_percent(interval=None)
                ram_pct = psutil.virtual_memory().percent
                writer.writerow(
                    [wall_time, gpu_util, gpu_mem_util, gpu_mem_used, gpu_mem_total, cpu_pct, ram_pct]
                )
                f.flush()
                time.sleep(INTERVAL_S)
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    main()
