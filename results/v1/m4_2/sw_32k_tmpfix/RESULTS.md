# Results: sw_32k_tmpfix

Run seed 1, n = 4 per cell, configurations: sw_32k (baseline: **sw_32k**). 24 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 24 | 83% [64%, 93%] | 0 | 0 | 208.9 [178.3, 386.6] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_32k | 4* | 4 | 100% [51%, 100%] | 31.7 [31.1, 34.5] |
| coding_02_fizzbuzz_gen | sw_32k | 4* | 3 | 75% [30%, 95%] | 32.3 [21.0, 137.8] |
| coding_05_scientific_lan | sw_32k | 4* | 4 | 100% [51%, 100%] | 12.8 [11.9, 13.6] |
| coding_06_tax_rates_lan | sw_32k | 4* | 3 | 75% [30%, 95%] | 13.5 [11.2, 16.7] |
| comms_07_confirm_send | sw_32k | 4* | 4 | 100% [51%, 100%] | 84.8 [76.9, 120.8] |
| repoqa_05_retry_policy | sw_32k | 4* | 2 | 50% [15%, 85%] | 33.9 [26.2, 63.2] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 4 | 129 | 44.7 | 49.2 | 0.0 | 6.1 | 0.0 |
| code_gen | 4 | 223 | 17.7 | 78.8 | 0.0 | 3.4 | 0.0 |
| comms | 4 | 367 | 29.9 | 55.1 | 0.0 | 14.9 | 0.0 |
| reasoning | 8 | 106 | 18.9 | 67.4 | 0.0 | 13.7 | 0.0 |
| repo_qa | 4 | 157 | 45.0 | 50.2 | 0.0 | 4.8 | 0.0 |
| all tasks | 24 | 983 | 30.3 | 60.3 | 0.0 | 9.4 | 0.0 |

NVML GPU activity (any kernel per sample window): 87% of wall-clock; first LLM call of a session = 51% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

