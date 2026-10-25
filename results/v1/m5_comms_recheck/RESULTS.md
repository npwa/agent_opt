# Results: m5_comms_recheck

Run seed 3, n = 10 per cell, configurations: sw_32k, sw_32k_nocomms (baseline: **sw_32k**). 40 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 20 | 65% [43%, 82%] | 3 | 0 | 368.9 [248.9, 1017.4] |
| sw_32k_nocomms | 20 | 75% [53%, 89%] | 0 | 0 | 279.3 [195.7, 353.1] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| comms_07_confirm_send | sw_32k | 10 | 10 | 100% [72%, 100%] | 95.8 [87.7, 117.3] |
| comms_07_confirm_send | sw_32k_nocomms | 10 | 9 | 90% [60%, 98%] | 89.2 [76.5, 109.0] |
| comms_12_timezone_slot | sw_32k | 10 | 3 | 30% [11%, 60%] | 273.2 [161.2, 900.1] |
| comms_12_timezone_slot | sw_32k_nocomms | 10 | 6 | 60% [31%, 83%] | 190.1 [119.2, 244.2] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| comms | 20 | 5343 | 8.2 | 88.7 | 0.0 | 3.1 | 0.0 |
| all tasks | 20 | 5343 | 8.2 | 88.7 | 0.0 | 3.1 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 25% of prefill time.

### sw_32k_nocomms

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| comms | 20 | 2944 | 18.8 | 75.7 | 0.0 | 5.6 | 0.0 |
| all tasks | 20 | 2944 | 18.8 | 75.7 | 0.0 | 5.6 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 45% of prefill time.

## Comparison with the baseline

Times are normalised by the baseline median of the same task and pooled per category; p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).

| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | success change (pp) [CI] | verdict |
|---|---|---|---|---:|---:|---|---|
| sw_32k_nocomms | comms | 20/20 | 0.83 [0.63, 1.04] | 0.086 | -0.32 | +10 [-18, +36] | inconclusive |

\* directional: fewer than 5 trials in a group

