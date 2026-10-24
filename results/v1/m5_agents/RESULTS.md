# Results: m5_agents

Run seed 1, n = 3 per cell, configurations: sw_32k, sw_32k_lean, sw_32k_verify (baseline: **sw_32k**). 81 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 27 | 56% [37%, 72%] | 3 | 0 | 2023.8 [884.4, 2324.1] |
| sw_32k_lean | 27 | 59% [41%, 75%] | 2 | 0 | 917.8 [614.2, 2247.6] |
| sw_32k_verify | 27 | 74% [55%, 87%] | 0 | 0 | 1076.2 [729.8, 1788.2] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_09_booking_slots | sw_32k | 3* | 0 | 0% [0%, 56%] | 122.3 [75.8, 137.7] |
| bugfix_09_booking_slots | sw_32k_lean | 3* | 0 | 0% [0%, 56%] | 121.4 [106.6, 193.6] |
| bugfix_09_booking_slots | sw_32k_verify | 3* | 2 | 67% [21%, 94%] | 180.7 [119.1, 325.6] |
| c_ringbuf_03_ringbuffer | sw_32k | 3* | 1 | 33% [6%, 79%] | 1069.8 [97.5, 1140.1] |
| c_ringbuf_03_ringbuffer | sw_32k_lean | 3* | 1 | 33% [6%, 79%] | 256.5 [114.5, 1140.1] |
| c_ringbuf_03_ringbuffer | sw_32k_verify | 3* | 3 | 100% [44%, 100%] | 329.0 [159.3, 679.8] |
| coding_02_fizzbuzz_gen | sw_32k | 3* | 3 | 100% [44%, 100%] | 39.2 [36.0, 123.9] |
| coding_02_fizzbuzz_gen | sw_32k_lean | 3* | 3 | 100% [44%, 100%] | 36.7 [35.4, 61.7] |
| coding_02_fizzbuzz_gen | sw_32k_verify | 3* | 3 | 100% [44%, 100%] | 47.4 [42.5, 78.2] |
| coding_03_compress_gen | sw_32k | 3* | 2 | 67% [21%, 94%] | 49.5 [32.4, 162.1] |
| coding_03_compress_gen | sw_32k_lean | 3* | 3 | 100% [44%, 100%] | 67.1 [32.1, 68.8] |
| coding_03_compress_gen | sw_32k_verify | 3* | 3 | 100% [44%, 100%] | 208.8 [172.2, 299.4] |
| coding_05_scientific_lan | sw_32k | 3* | 3 | 100% [44%, 100%] | 18.3 [18.1, 18.3] |
| coding_05_scientific_lan | sw_32k_lean | 3* | 3 | 100% [44%, 100%] | 18.0 [16.5, 23.5] |
| coding_05_scientific_lan | sw_32k_verify | 3* | 3 | 100% [44%, 100%] | 18.4 [18.4, 44.6] |
| coding_06_tax_rates_lan | sw_32k | 3* | 1 | 33% [6%, 79%] | 20.3 [18.6, 22.3] |
| coding_06_tax_rates_lan | sw_32k_lean | 3* | 1 | 33% [6%, 79%] | 12.7 [11.0, 13.6] |
| coding_06_tax_rates_lan | sw_32k_verify | 3* | 0 | 0% [0%, 56%] | 14.4 [13.4, 14.8] |
| coding_10_wcx_constrained | sw_32k | 3* | 1 | 33% [6%, 79%] | 600.1 [530.8, 600.1] |
| coding_10_wcx_constrained | sw_32k_lean | 3* | 2 | 67% [21%, 94%] | 312.5 [207.1, 600.1] |
| coding_10_wcx_constrained | sw_32k_verify | 3* | 2 | 67% [21%, 94%] | 132.0 [91.0, 179.4] |
| refactor_02_rename_price | sw_32k | 3* | 3 | 100% [44%, 100%] | 66.9 [48.8, 78.2] |
| refactor_02_rename_price | sw_32k_lean | 3* | 1 | 33% [6%, 79%] | 64.3 [63.6, 112.0] |
| refactor_02_rename_price | sw_32k_verify | 3* | 3 | 100% [44%, 100%] | 111.1 [84.8, 131.6] |
| repoqa_05_retry_policy | sw_32k | 3* | 1 | 33% [6%, 79%] | 37.4 [26.4, 41.4] |
| repoqa_05_retry_policy | sw_32k_lean | 3* | 2 | 67% [21%, 94%] | 28.6 [27.4, 34.2] |
| repoqa_05_retry_policy | sw_32k_verify | 3* | 1 | 33% [6%, 79%] | 34.4 [29.1, 34.8] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 336 | 31.9 | 66.1 | 0.0 | 2.0 | 0.0 |
| code_gen | 12 | 4482 | 11.0 | 83.1 | 5.4 | 0.6 | 0.0 |
| reasoning | 6 | 116 | 50.2 | 40.5 | 0.0 | 9.3 | 0.0 |
| refactor | 3 | 194 | 42.6 | 54.2 | 0.0 | 3.2 | 0.0 |
| repo_qa | 3 | 105 | 41.3 | 53.3 | 0.0 | 5.5 | 0.0 |
| all tasks | 27 | 5233 | 15.0 | 79.4 | 4.6 | 1.0 | 0.0 |

NVML GPU activity (any kernel per sample window): 93% of wall-clock; first LLM call of a session = 52% of prefill time.

### sw_32k_lean

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 422 | 21.7 | 76.7 | 0.0 | 1.6 | 0.0 |
| code_gen | 12 | 2933 | 12.9 | 86.2 | 0.0 | 0.9 | 0.0 |
| reasoning | 6 | 95 | 15.4 | 73.0 | 0.0 | 11.6 | 0.0 |
| refactor | 3 | 240 | 25.6 | 71.9 | 0.0 | 2.5 | 0.0 |
| repo_qa | 3 | 90 | 31.6 | 61.9 | 0.0 | 6.4 | 0.0 |
| all tasks | 27 | 3780 | 15.2 | 83.4 | 0.0 | 1.4 | 0.0 |

NVML GPU activity (any kernel per sample window): 93% of wall-clock; first LLM call of a session = 34% of prefill time.

### sw_32k_verify

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 625 | 19.5 | 79.4 | 0.0 | 1.1 | 0.0 |
| code_gen | 12 | 2419 | 14.3 | 79.6 | 5.0 | 1.1 | 0.0 |
| reasoning | 6 | 124 | 30.1 | 60.9 | 0.0 | 9.1 | 0.0 |
| refactor | 3 | 328 | 25.0 | 72.8 | 0.0 | 2.2 | 0.0 |
| repo_qa | 3 | 98 | 31.0 | 63.0 | 0.0 | 6.0 | 0.0 |
| all tasks | 27 | 3595 | 17.2 | 77.8 | 3.4 | 1.6 | 0.0 |

NVML GPU activity (any kernel per sample window): 92% of wall-clock; first LLM call of a session = 34% of prefill time.

## Comparison with the baseline

Times are normalised by the baseline median of the same task and pooled per category; p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).

| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | success change (pp) [CI] | verdict |
|---|---|---|---|---:|---:|---|---|
| sw_32k_lean | bugfix | 3/3* | 0.99 [0.77, 2.55] | 1.000 | +0.11 | +0 [-56, +56] | inconclusive |
| sw_32k_lean | code_gen | 12/12 | 0.92 [0.43, 1.21] | 1.000 | -0.19 | +17 [-19, +48] | inconclusive |
| sw_32k_lean | reasoning | 6/6 | 0.79 [0.58, 1.13] | 0.904 | -0.61 | +0 [-44, +44] | inconclusive |
| sw_32k_lean | refactor | 3/3* | 0.96 [0.81, 2.30] | 1.000 | +0.11 | -67 [-94, +6] | inconclusive |
| sw_32k_lean | repo_qa | 3/3* | 0.76 [0.66, 1.30] | 1.000 | -0.33 | +33 [-32, +72] | inconclusive |
| sw_32k_verify | bugfix | 3/3* | 1.48 [0.86, 4.30] | 1.000 | +0.56 | +67 [-6, +94] | inconclusive |
| sw_32k_verify | code_gen | 12/12 | 0.86 [0.26, 2.74] | 1.000 | +0.00 | +33 [-2, +61] | inconclusive |
| sw_32k_verify | reasoning | 6/6 | 0.87 [0.68, 1.72] | 1.000 | -0.11 | -17 [-56, +32] | inconclusive |
| sw_32k_verify | refactor | 3/3* | 1.66 [1.08, 2.70] | 0.904 | +1.00 | +0 [-56, +56] | negative |
| sw_32k_verify | repo_qa | 3/3* | 0.92 [0.70, 1.32] | 1.000 | -0.33 | +0 [-53, +53] | inconclusive |

\* directional: fewer than 5 trials in a group

