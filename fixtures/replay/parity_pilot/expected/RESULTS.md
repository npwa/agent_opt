# Results: parity_pilot

Run seed 21, n = 5 per cell, configurations: baseline_32k (baseline: **baseline_32k**). 15 measured trials, 0 harness failures excluded, 15 uninstrumented parity trials (see parity). Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| baseline_32k | 15 | 60% [36%, 80%] | 0 | 0 | 248.9 [228.1, 491.8] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| coding_01_my_sqrt_review | baseline_32k | 5 | 3 | 60% [23%, 88%] | 120.8 [109.3, 337.2] |
| coding_05_scientific_lan | baseline_32k | 5 | 5 | 100% [57%, 100%] | 63.1 [57.0, 77.8] |
| coding_06_tax_rates_lan | baseline_32k | 5 | 1 | 20% [4%, 62%] | 65.0 [61.8, 76.8] |

\* directional: fewer than 5 trials

## Where the time goes

No traces in this run (run `python -m harness.trace --run <dir>` first).

## Comparison with the baseline

Only one configuration: nothing to compare.

## Instrumentation parity

Verdict: **inconclusive** (15 uninstrumented trials). See `parity.md`.

