# Results: m5_trimtools

Run seed 5, n = 5 per cell, configurations: sw_32k_trimtools (baseline: **sw_32k_trimtools**). 40 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k_trimtools | 40 | 68% [52%, 80%] | 0 | 0 | 262.7 [213.6, 449.3] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 37.7 [32.9, 49.5] |
| coding_01_my_sqrt_review | sw_32k_trimtools | 5 | 1 | 20% [4%, 62%] | 37.6 [16.4, 54.5] |
| coding_02_fizzbuzz_gen | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 26.1 [20.3, 110.1] |
| coding_03_compress_gen | sw_32k_trimtools | 5 | 4 | 80% [38%, 96%] | 29.8 [21.5, 31.6] |
| coding_05_scientific_lan | sw_32k_trimtools | 5 | 5 | 100% [57%, 100%] | 11.1 [9.4, 30.0] |
| coding_06_tax_rates_lan | sw_32k_trimtools | 5 | 0 | 0% [0%, 43%] | 14.3 [13.8, 14.7] |
| refactor_02_rename_price | sw_32k_trimtools | 5 | 4 | 80% [38%, 96%] | 76.4 [75.0, 116.6] |
| repoqa_05_retry_policy | sw_32k_trimtools | 5 | 3 | 60% [23%, 88%] | 29.7 [24.3, 42.3] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k_trimtools

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 5 | 194 | 40.5 | 54.3 | 0.0 | 5.2 | 0.0 |
| code_gen | 10 | 349 | 28.0 | 66.4 | 0.0 | 5.6 | 0.0 |
| code_review | 5 | 174 | 14.3 | 80.5 | 0.0 | 5.2 | 0.0 |
| reasoning | 10 | 144 | 31.1 | 56.1 | 0.0 | 12.8 | 0.0 |
| refactor | 5 | 451 | 28.8 | 68.9 | 0.0 | 2.3 | 0.0 |
| repo_qa | 5 | 156 | 33.4 | 60.4 | 0.0 | 6.2 | 0.0 |
| all tasks | 40 | 1468 | 29.1 | 65.6 | 0.0 | 5.2 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 47% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

