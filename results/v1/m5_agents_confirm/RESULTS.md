# Results: m5_agents_confirm

Run seed 2, n = 10 per cell, configurations: sw_32k, sw_32k_verify (baseline: **sw_32k**). 80 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 40 | 48% [33%, 63%] | 0 | 0 | 640.8 [422.0, 965.3] |
| sw_32k_verify | 40 | 45% [31%, 60%] | 5 | 0 | 728.9 [421.6, 1775.1] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_09_booking_slots | sw_32k | 10 | 4 | 40% [17%, 69%] | 103.2 [77.3, 157.4] |
| bugfix_09_booking_slots | sw_32k_verify | 10 | 6 | 60% [31%, 83%] | 170.8 [92.0, 274.5] |
| c_ringbuf_03_ringbuffer | sw_32k | 10 | 8 | 80% [49%, 94%] | 320.7 [200.7, 411.5] |
| c_ringbuf_03_ringbuffer | sw_32k_verify | 10 | 8 | 80% [49%, 94%] | 386.2 [200.6, 884.8] |
| coding_06_tax_rates_lan | sw_32k | 10 | 2 | 20% [6%, 51%] | 20.6 [14.1, 21.4] |
| coding_06_tax_rates_lan | sw_32k_verify | 10 | 1 | 10% [2%, 40%] | 14.6 [13.3, 15.7] |
| coding_10_wcx_constrained | sw_32k | 10 | 5 | 50% [24%, 76%] | 196.3 [129.9, 375.1] |
| coding_10_wcx_constrained | sw_32k_verify | 10 | 3 | 30% [11%, 60%] | 157.4 [115.7, 600.1] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 10 | 1204 | 31.0 | 67.3 | 0.0 | 1.7 | 0.0 |
| code_gen | 20 | 5705 | 19.7 | 75.3 | 4.2 | 0.8 | 0.0 |
| reasoning | 10 | 184 | 41.0 | 49.0 | 0.0 | 10.0 | 0.0 |
| all tasks | 40 | 7092 | 22.2 | 73.2 | 3.4 | 1.2 | 0.0 |

NVML GPU activity (any kernel per sample window): 93% of wall-clock; first LLM call of a session = 42% of prefill time.

### sw_32k_verify

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 10 | 1862 | 20.5 | 78.3 | 0.0 | 1.2 | 0.0 |
| code_gen | 20 | 8247 | 11.4 | 85.1 | 2.9 | 0.6 | 0.0 |
| reasoning | 10 | 145 | 17.9 | 69.4 | 0.0 | 12.7 | 0.0 |
| all tasks | 40 | 10255 | 13.1 | 83.7 | 2.4 | 0.8 | 0.0 |

NVML GPU activity (any kernel per sample window): 93% of wall-clock; first LLM call of a session = 36% of prefill time; 1 trial(s) flagged by the decomposition checks.

## Comparison with the baseline

Times are normalised by the baseline median of the same task and pooled per category; p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).

| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | success change (pp) [CI] | verdict |
|---|---|---|---|---:|---:|---|---|
| sw_32k_verify | bugfix | 10/10 | 1.66 [0.77, 2.95] | 0.483 | +0.32 | +20 [-21, +53] | inconclusive |
| sw_32k_verify | code_gen | 20/20 | 0.93 [0.58, 2.92] | 0.490 | +0.13 | -10 [-37, +19] | inconclusive |
| sw_32k_verify | reasoning | 10/10 | 0.70 [0.64, 1.05] | 0.192 | -0.50 | -10 [-42, +24] | inconclusive |

\* directional: fewer than 5 trials in a group

