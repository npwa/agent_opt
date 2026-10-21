# Results: baseline

Run seed 1, n = 10 per cell, configurations: baseline_32k (baseline: **baseline_32k**). 140 measured trials, 0 harness failures excluded, 28 uninstrumented parity trials (see parity). Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| baseline_32k | 140 | 74% [66%, 81%] | 0 | 0 | 1596.4 [1303.9, 2120.3] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | baseline_32k | 10 | 8 | 80% [49%, 94%] | 72.0 [66.1, 78.3] |
| c_ringbuf_03_ringbuffer | baseline_32k | 10 | 6 | 60% [31%, 83%] | 351.5 [212.5, 698.8] |
| coding_01_my_sqrt_review | baseline_32k | 10 | 6 | 60% [31%, 83%] | 77.3 [70.9, 85.8] |
| coding_02_fizzbuzz_gen | baseline_32k | 10 | 8 | 80% [49%, 94%] | 78.3 [69.2, 84.9] |
| coding_03_compress_gen | baseline_32k | 10 | 8 | 80% [49%, 94%] | 77.6 [65.0, 93.6] |
| coding_04_dir_stats_gen | baseline_32k | 10 | 10 | 100% [72%, 100%] | 81.2 [64.6, 95.3] |
| coding_05_scientific_lan | baseline_32k | 10 | 10 | 100% [72%, 100%] | 50.0 [49.0, 52.3] |
| coding_06_tax_rates_lan | baseline_32k | 10 | 3 | 30% [11%, 60%] | 50.9 [49.6, 52.5] |
| comms_07_confirm_send | baseline_32k | 10 | 10 | 100% [72%, 100%] | 111.7 [110.2, 113.9] |
| logtriage_04_pool_exhaustion | baseline_32k | 10 | 8 | 80% [49%, 94%] | 157.8 [142.2, 178.2] |
| longctx_06_tidal_report | baseline_32k | 10 | 10 | 100% [72%, 100%] | 101.4 [100.8, 102.0] |
| refactor_02_rename_price | baseline_32k | 10 | 9 | 90% [60%, 98%] | 111.2 [88.1, 123.5] |
| repoqa_05_retry_policy | baseline_32k | 10 | 7 | 70% [40%, 89%] | 67.2 [64.0, 76.7] |
| research_08e_free_will | baseline_32k | 10 | 1 | 10% [2%, 40%] | 208.2 [151.7, 284.5] |

\* directional: fewer than 5 trials

## Where the time goes

### baseline_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 10 | 723 | 70.0 | 27.2 | 0.0 | 2.8 | 0.0 |
| code_gen | 40 | 6879 | 36.6 | 60.3 | 1.8 | 1.3 | 0.0 |
| code_review | 10 | 767 | 57.0 | 40.4 | 0.0 | 2.5 | 0.0 |
| comms | 10 | 1160 | 48.3 | 39.9 | 0.0 | 11.8 | 0.0 |
| log_triage | 10 | 1599 | 65.3 | 33.2 | 0.0 | 1.5 | 0.0 |
| long_context | 10 | 1010 | 80.7 | 17.3 | 0.0 | 1.9 | 0.0 |
| reasoning | 20 | 1019 | 79.8 | 16.4 | 0.1 | 3.7 | 0.0 |
| refactor | 10 | 1096 | 53.9 | 44.1 | 0.0 | 1.9 | 0.0 |
| repo_qa | 10 | 707 | 68.0 | 29.2 | 0.1 | 2.8 | 0.0 |
| research | 10 | 2319 | 67.4 | 30.1 | 1.1 | 1.3 | 0.0 |
| all tasks | 140 | 17280 | 54.0 | 42.7 | 0.9 | 2.4 | 0.0 |

NVML GPU activity (any kernel per sample window): 95% of wall-clock; first LLM call of a session = 72% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

## Instrumentation parity

Verdict: **inconclusive** (28 uninstrumented trials). See `parity.md`.

