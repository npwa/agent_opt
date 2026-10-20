# Results: traced_pilot

Run seed 5, n = 1 per cell, configurations: baseline_32k (baseline: **baseline_32k**). 2 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| baseline_32k | 2 | 100% [34%, 100%] | 0 | 0 | 164.7 [164.7, 164.7] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | baseline_32k | 1* | 1 | 100% [21%, 100%] | 111.5 [111.5, 111.5] |
| coding_05_scientific_lan | baseline_32k | 1* | 1 | 100% [21%, 100%] | 53.2 [53.2, 53.2] |

\* directional: fewer than 5 trials

## Where the time goes

### baseline_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 1 | 112 | 72.8 | 25.1 | 0.0 | 2.1 | 0.0 |
| reasoning | 1 | 53 | 78.9 | 17.3 | 0.0 | 3.7 | 0.0 |
| all tasks | 2 | 165 | 74.8 | 22.6 | 0.0 | 2.6 | 0.0 |

NVML GPU activity (any kernel per sample window): 87% of wall-clock; first LLM call of a session = 92% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

