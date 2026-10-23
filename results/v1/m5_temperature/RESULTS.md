# Results: m5_temperature

Run seed 1, n = 3 per cell, configurations: sw_32k, sw_32k_t03, sw_32k_t06 (baseline: **sw_32k**). 126 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 42 | 60% [44%, 73%] | 1 | 0 | 1416.9 [883.9, 2642.6] |
| sw_32k_t03 | 42 | 60% [44%, 73%] | 0 | 0 | 1254.8 [1121.9, 2028.1] |
| sw_32k_t06 | 42 | 69% [54%, 81%] | 0 | 0 | 1797.9 [1418.1, 2322.5] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_09_booking_slots | sw_32k | 3* | 0 | 0% [0%, 56%] | 61.5 [60.4, 79.1] |
| bugfix_09_booking_slots | sw_32k_t03 | 3* | 0 | 0% [0%, 56%] | 61.5 [58.1, 69.3] |
| bugfix_09_booking_slots | sw_32k_t06 | 3* | 0 | 0% [0%, 56%] | 86.4 [65.7, 105.9] |
| c_ringbuf_03_ringbuffer | sw_32k | 3* | 2 | 67% [21%, 94%] | 462.0 [160.0, 1140.0] |
| c_ringbuf_03_ringbuffer | sw_32k_t03 | 3* | 2 | 67% [21%, 94%] | 258.0 [227.5, 725.8] |
| c_ringbuf_03_ringbuffer | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 713.4 [599.8, 835.2] |
| coding_01_my_sqrt_review | sw_32k | 3* | 3 | 100% [44%, 100%] | 31.1 [25.5, 44.2] |
| coding_01_my_sqrt_review | sw_32k_t03 | 3* | 1 | 33% [6%, 79%] | 37.8 [32.3, 38.1] |
| coding_01_my_sqrt_review | sw_32k_t06 | 3* | 2 | 67% [21%, 94%] | 35.6 [33.2, 40.8] |
| coding_02_fizzbuzz_gen | sw_32k | 3* | 3 | 100% [44%, 100%] | 28.0 [24.6, 43.6] |
| coding_02_fizzbuzz_gen | sw_32k_t03 | 3* | 3 | 100% [44%, 100%] | 28.9 [26.6, 34.2] |
| coding_02_fizzbuzz_gen | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 27.4 [25.3, 32.2] |
| coding_03_compress_gen | sw_32k | 3* | 2 | 67% [21%, 94%] | 39.5 [37.5, 44.1] |
| coding_03_compress_gen | sw_32k_t03 | 3* | 3 | 100% [44%, 100%] | 26.0 [19.7, 65.6] |
| coding_03_compress_gen | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 23.5 [18.0, 26.5] |
| coding_05_scientific_lan | sw_32k | 3* | 3 | 100% [44%, 100%] | 14.0 [12.9, 14.6] |
| coding_05_scientific_lan | sw_32k_t03 | 3* | 3 | 100% [44%, 100%] | 14.2 [12.7, 14.3] |
| coding_05_scientific_lan | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 9.7 [9.7, 12.3] |
| coding_06_tax_rates_lan | sw_32k | 3* | 0 | 0% [0%, 56%] | 16.3 [12.5, 16.7] |
| coding_06_tax_rates_lan | sw_32k_t03 | 3* | 1 | 33% [6%, 79%] | 13.5 [13.4, 14.5] |
| coding_06_tax_rates_lan | sw_32k_t06 | 3* | 0 | 0% [0%, 56%] | 14.9 [13.4, 16.5] |
| coding_10_wcx_constrained | sw_32k | 3* | 0 | 0% [0%, 56%] | 98.1 [79.2, 326.2] |
| coding_10_wcx_constrained | sw_32k_t03 | 3* | 2 | 67% [21%, 94%] | 62.5 [43.8, 182.1] |
| coding_10_wcx_constrained | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 149.2 [47.6, 312.9] |
| comms_12_timezone_slot | sw_32k | 3* | 3 | 100% [44%, 100%] | 160.9 [141.5, 211.4] |
| comms_12_timezone_slot | sw_32k_t03 | 3* | 1 | 33% [6%, 79%] | 120.9 [113.5, 126.2] |
| comms_12_timezone_slot | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 134.0 [129.0, 206.6] |
| logtriage_04_pool_exhaustion | sw_32k | 3* | 2 | 67% [21%, 94%] | 135.7 [49.6, 138.4] |
| logtriage_04_pool_exhaustion | sw_32k_t03 | 3* | 2 | 67% [21%, 94%] | 162.1 [126.1, 278.7] |
| logtriage_04_pool_exhaustion | sw_32k_t06 | 3* | 1 | 33% [6%, 79%] | 157.1 [88.0, 239.0] |
| longctx_11_harbor_report | sw_32k | 3* | 1 | 33% [6%, 79%] | 109.3 [105.4, 140.7] |
| longctx_11_harbor_report | sw_32k_t03 | 3* | 3 | 100% [44%, 100%] | 120.6 [113.6, 124.2] |
| longctx_11_harbor_report | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 116.3 [115.4, 119.3] |
| refactor_02_rename_price | sw_32k | 3* | 3 | 100% [44%, 100%] | 72.8 [36.1, 116.1] |
| refactor_02_rename_price | sw_32k_t03 | 3* | 2 | 67% [21%, 94%] | 69.0 [65.2, 69.2] |
| refactor_02_rename_price | sw_32k_t06 | 3* | 3 | 100% [44%, 100%] | 75.4 [74.4, 110.6] |
| repoqa_05_retry_policy | sw_32k | 3* | 2 | 67% [21%, 94%] | 26.4 [22.6, 61.1] |
| repoqa_05_retry_policy | sw_32k_t03 | 3* | 1 | 33% [6%, 79%] | 29.0 [23.9, 29.1] |
| repoqa_05_retry_policy | sw_32k_t06 | 3* | 1 | 33% [6%, 79%] | 24.0 [24.0, 30.4] |
| research_08e_free_will | sw_32k | 3* | 1 | 33% [6%, 79%] | 161.3 [116.1, 266.4] |
| research_08e_free_will | sw_32k_t03 | 3* | 1 | 33% [6%, 79%] | 250.8 [245.5, 256.8] |
| research_08e_free_will | sw_32k_t06 | 3* | 1 | 33% [6%, 79%] | 231.0 [174.6, 234.3] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 201 | 46.6 | 50.3 | 0.0 | 3.1 | 0.0 |
| code_gen | 12 | 2483 | 10.6 | 78.7 | 9.7 | 1.0 | 0.0 |
| code_review | 3 | 101 | 15.0 | 79.5 | 0.0 | 5.5 | 0.0 |
| comms | 3 | 514 | 19.1 | 79.3 | 0.0 | 1.6 | 0.0 |
| log_triage | 3 | 325 | 57.9 | 40.0 | 0.0 | 2.1 | 0.0 |
| long_context | 3 | 355 | 56.6 | 41.8 | 0.0 | 1.6 | 0.0 |
| reasoning | 6 | 87 | 18.4 | 69.5 | 0.0 | 12.1 | 0.0 |
| refactor | 3 | 225 | 29.6 | 67.7 | 0.0 | 2.7 | 0.0 |
| repo_qa | 3 | 110 | 56.2 | 38.7 | 0.0 | 5.1 | 0.0 |
| research | 3 | 544 | 57.1 | 38.2 | 3.0 | 1.7 | 0.0 |
| all tasks | 42 | 4945 | 26.6 | 66.4 | 5.2 | 1.8 | 0.0 |

NVML GPU activity (any kernel per sample window): 92% of wall-clock; first LLM call of a session = 36% of prefill time.

### sw_32k_t03

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 189 | 44.5 | 52.2 | 0.0 | 3.2 | 0.0 |
| code_gen | 12 | 1701 | 18.5 | 80.0 | 0.0 | 1.5 | 0.0 |
| code_review | 3 | 108 | 13.7 | 81.2 | 0.0 | 5.1 | 0.0 |
| comms | 3 | 361 | 27.8 | 70.2 | 0.0 | 2.0 | 0.0 |
| log_triage | 3 | 568 | 67.3 | 31.4 | 0.0 | 1.3 | 0.0 |
| long_context | 3 | 358 | 58.1 | 40.3 | 0.0 | 1.6 | 0.0 |
| reasoning | 6 | 83 | 19.0 | 67.8 | 0.0 | 13.1 | 0.0 |
| refactor | 3 | 203 | 31.4 | 65.7 | 0.0 | 2.9 | 0.0 |
| repo_qa | 3 | 82 | 40.7 | 52.4 | 0.0 | 7.0 | 0.0 |
| research | 3 | 753 | 75.0 | 21.9 | 1.7 | 1.4 | 0.0 |
| all tasks | 42 | 4406 | 40.4 | 57.2 | 0.3 | 2.0 | 0.0 |

NVML GPU activity (any kernel per sample window): 93% of wall-clock; first LLM call of a session = 34% of prefill time.

### sw_32k_t06

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 258 | 39.5 | 58.0 | 0.0 | 2.4 | 0.0 |
| code_gen | 12 | 2811 | 13.6 | 74.9 | 10.5 | 1.0 | 0.0 |
| code_review | 3 | 110 | 14.1 | 80.8 | 0.0 | 5.0 | 0.0 |
| comms | 3 | 470 | 21.2 | 77.2 | 0.0 | 1.7 | 0.0 |
| log_triage | 3 | 485 | 63.6 | 35.0 | 0.0 | 1.4 | 0.0 |
| long_context | 3 | 351 | 59.2 | 39.1 | 0.0 | 1.6 | 0.0 |
| reasoning | 6 | 77 | 20.8 | 65.1 | 0.0 | 14.1 | 0.0 |
| refactor | 3 | 261 | 35.7 | 62.1 | 0.0 | 2.3 | 0.0 |
| repo_qa | 3 | 78 | 37.2 | 55.6 | 0.0 | 7.3 | 0.0 |
| research | 3 | 640 | 67.1 | 30.1 | 1.3 | 1.4 | 0.0 |
| all tasks | 42 | 5540 | 30.4 | 62.5 | 5.5 | 1.7 | 0.0 |

NVML GPU activity (any kernel per sample window): 92% of wall-clock; first LLM call of a session = 34% of prefill time.

## Comparison with the baseline

Times are normalised by the baseline median of the same task and pooled per category; p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).

| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | success change (pp) [CI] | verdict |
|---|---|---|---|---:|---:|---|---|
| sw_32k_t03 | bugfix | 3/3* | 1.00 [0.73, 1.15] | 1.000 | -0.22 | +0 [-56, +56] | inconclusive |
| sw_32k_t03 | code_gen | 12/12 | 0.80 [0.50, 1.40] | 1.000 | -0.21 | +25 [-11, +54] | inconclusive |
| sw_32k_t03 | code_review | 3/3* | 1.22 [0.73, 1.49] | 1.000 | +0.33 | -67 [-94, +6] | inconclusive |
| sw_32k_t03 | comms | 3/3* | 0.75 [0.54, 0.89] | 1.000 | -1.00 | -67 [-94, +6] | inconclusive |
| sw_32k_t03 | log_triage | 3/3* | 1.19 [0.91, 5.62] | 1.000 | +0.56 | +0 [-53, +53] | inconclusive |
| sw_32k_t03 | long_context | 3/3* | 1.10 [0.81, 1.18] | 1.000 | +0.33 | +67 [-6, +94] | inconclusive |
| sw_32k_t03 | reasoning | 6/6 | 0.90 [0.82, 1.09] | 1.000 | -0.33 | +17 [-32, +56] | inconclusive |
| sw_32k_t03 | refactor | 3/3* | 0.95 [0.56, 1.92] | 1.000 | -0.33 | -33 [-79, +29] | inconclusive |
| sw_32k_t03 | repo_qa | 3/3* | 1.10 [0.39, 1.29] | 1.000 | +0.11 | -33 [-72, +32] | inconclusive |
| sw_32k_t03 | research | 3/3* | 1.55 [0.92, 2.21] | 1.000 | +0.33 | +0 [-53, +53] | inconclusive |
| sw_32k_t06 | bugfix | 3/3* | 1.40 [0.83, 1.75] | 1.000 | +0.78 | +0 [-56, +56] | inconclusive |
| sw_32k_t06 | code_gen | 12/12 | 1.06 [0.59, 1.54] | 1.000 | -0.06 | +42 [+9, +68] | inconclusive |
| sw_32k_t06 | code_review | 3/3* | 1.14 [0.75, 1.60] | 1.000 | +0.33 | -33 [-79, +29] | inconclusive |
| sw_32k_t06 | comms | 3/3* | 0.83 [0.61, 1.46] | 1.000 | -0.56 | +0 [-56, +56] | inconclusive |
| sw_32k_t06 | log_triage | 3/3* | 1.16 [0.64, 4.82] | 1.000 | +0.56 | -33 [-72, +32] | inconclusive |
| sw_32k_t06 | long_context | 3/3* | 1.06 [0.82, 1.13] | 1.000 | +0.33 | +67 [-6, +94] | inconclusive |
| sw_32k_t06 | reasoning | 6/6 | 0.85 [0.68, 1.03] | 1.000 | -0.61 | +0 [-44, +44] | inconclusive |
| sw_32k_t06 | refactor | 3/3* | 1.04 [0.64, 3.06] | 1.000 | +0.33 | +0 [-56, +56] | inconclusive |
| sw_32k_t06 | repo_qa | 3/3* | 0.91 [0.39, 1.35] | 1.000 | -0.11 | -33 [-72, +32] | inconclusive |
| sw_32k_t06 | research | 3/3* | 1.43 [0.66, 2.02] | 1.000 | +0.33 | +0 [-53, +53] | inconclusive |

\* directional: fewer than 5 trials in a group

