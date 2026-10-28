# Results: m5_trimtools

Run seed 5, n = 5 per cell, configurations: sw_32k, sw_32k_trimtools (baseline: **sw_32k**). 80 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 40 | 75% [60%, 86%] | 0 | 0 | 255.5 [207.3, 424.3] |
| sw_32k_trimtools | 40 | 75% [60%, 86%] | 1 | 0 | 260.8 [202.6, 946.1] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_32k | 5 | 5 | 100% [57%, 100%] | 31.0 [29.9, 33.7] |
| bugfix_01_moving_average | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 36.2 [33.4, 72.6] |
| coding_01_my_sqrt_review | sw_32k | 5 | 3 | 60% [23%, 88%] | 34.8 [22.0, 126.1] |
| coding_01_my_sqrt_review | sw_32k_trimtools | 5 | 2 | 40% [12%, 77%] | 38.0 [21.4, 50.2] |
| coding_02_fizzbuzz_gen | sw_32k | 5 | 4 | 80% [38%, 96%] | 26.5 [13.3, 44.3] |
| coding_02_fizzbuzz_gen | sw_32k_trimtools | 5 | 4 | 80% [38%, 96%] | 30.2 [23.7, 600.1] |
| coding_03_compress_gen | sw_32k | 5 | 4 | 80% [38%, 96%] | 44.4 [37.4, 62.4] |
| coding_03_compress_gen | sw_32k_trimtools | 5 | 3 | 60% [23%, 88%] | 28.1 [24.4, 46.3] |
| coding_05_scientific_lan | sw_32k | 5 | 5 | 100% [57%, 100%] | 12.1 [9.0, 12.3] |
| coding_05_scientific_lan | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 11.8 [9.6, 12.9] |
| coding_06_tax_rates_lan | sw_32k | 5 | 3 | 60% [23%, 88%] | 14.0 [12.9, 14.5] |
| coding_06_tax_rates_lan | sw_32k_trimtools | 5 | 2 | 40% [12%, 77%] | 13.7 [10.7, 14.6] |
| refactor_02_rename_price | sw_32k | 5 | 4 | 80% [38%, 96%] | 65.7 [60.6, 95.1] |
| refactor_02_rename_price | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 73.2 [59.4, 97.7] |
| repoqa_05_retry_policy | sw_32k | 5 | 2 | 40% [12%, 77%] | 27.0 [22.2, 35.9] |
| repoqa_05_retry_policy | sw_32k_trimtools | 5 | 4 | 80% [38%, 96%] | 29.6 [20.0, 51.7] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 5 | 157 | 53.2 | 40.4 | 0.0 | 6.4 | 0.0 |
| code_gen | 10 | 369 | 33.4 | 61.4 | 0.0 | 5.2 | 0.0 |
| code_review | 5 | 254 | 14.9 | 81.4 | 0.0 | 3.8 | 0.0 |
| reasoning | 10 | 126 | 20.5 | 65.0 | 0.0 | 14.4 | 0.0 |
| refactor | 5 | 354 | 36.1 | 61.0 | 0.0 | 2.9 | 0.0 |
| repo_qa | 5 | 142 | 37.8 | 55.5 | 0.0 | 6.7 | 0.0 |
| all tasks | 40 | 1402 | 32.2 | 62.3 | 0.0 | 5.5 | 0.0 |

NVML GPU activity (any kernel per sample window): 89% of wall-clock; first LLM call of a session = 48% of prefill time.

### sw_32k_trimtools

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 5 | 219 | 42.5 | 52.8 | 0.0 | 4.7 | 0.0 |
| code_gen | 10 | 888 | 8.7 | 89.2 | 0.0 | 2.1 | 0.0 |
| code_review | 5 | 187 | 13.3 | 81.7 | 0.0 | 5.0 | 0.0 |
| reasoning | 10 | 124 | 21.1 | 64.0 | 0.0 | 14.8 | 0.0 |
| refactor | 5 | 370 | 37.0 | 60.2 | 0.0 | 2.8 | 0.0 |
| repo_qa | 5 | 173 | 59.6 | 34.8 | 0.0 | 5.6 | 0.0 |
| all tasks | 40 | 1961 | 23.5 | 72.6 | 0.0 | 3.9 | 0.0 |

NVML GPU activity (any kernel per sample window): 89% of wall-clock; first LLM call of a session = 50% of prefill time.

## Comparison with the baseline

Times are normalised by the baseline median of the same task and pooled per category; p = Mann-Whitney U, Holm-adjusted over this table; CIs are 95% (bootstrap for time, Newcombe for rates).

| Configuration | Category | n base/var | time ratio (median) [CI] | p (Holm) | effect (rank-biserial) | success change (pp) [CI] | verdict |
|---|---|---|---|---:|---:|---|---|
| sw_32k_trimtools | bugfix | 5/5 | 1.17 [1.04, 2.34] | 0.095 | +0.92 | +0 [-43, +43] | negative |
| sw_32k_trimtools | code_gen | 10/10 | 0.92 [0.61, 1.52] | 1.000 | -0.10 | -10 [-44, +26] | inconclusive |
| sw_32k_trimtools | code_review | 5/5 | 1.09 [0.30, 1.73] | 1.000 | +0.04 | -20 [-60, +32] | inconclusive |
| sw_32k_trimtools | reasoning | 10/10 | 0.98 [0.88, 1.08] | 1.000 | +0.04 | -10 [-44, +26] | inconclusive |
| sw_32k_trimtools | refactor | 5/5 | 1.11 [0.77, 1.49] | 1.000 | +0.12 | +20 [-26, +62] | inconclusive |
| sw_32k_trimtools | repo_qa | 5/5 | 1.10 [0.65, 2.12] | 1.000 | +0.12 | +40 [-16, +73] | inconclusive |

\* directional: fewer than 5 trials in a group

