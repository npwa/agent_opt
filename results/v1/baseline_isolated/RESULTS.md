# Results: baseline_isolated

Run seed 1, n = 10 per cell, configurations: baseline_32k (baseline: **baseline_32k**). 140 measured trials, 0 harness failures excluded, 28 uninstrumented parity trials (see parity). Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| baseline_32k | 140 | 71% [63%, 78%] | 0 | 0 | 1239.0 [746.8, 1805.7] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | baseline_32k | 10 | 10 | 100% [72%, 100%] | 33.5 [30.1, 42.0] |
| c_ringbuf_03_ringbuffer | baseline_32k | 10 | 3 | 30% [11%, 60%] | 401.1 [150.8, 735.0] |
| coding_01_my_sqrt_review | baseline_32k | 10 | 5 | 50% [24%, 76%] | 43.7 [32.6, 50.2] |
| coding_02_fizzbuzz_gen | baseline_32k | 10 | 8 | 80% [49%, 94%] | 24.4 [16.3, 42.4] |
| coding_03_compress_gen | baseline_32k | 10 | 6 | 60% [31%, 83%] | 45.0 [28.2, 85.1] |
| coding_04_dir_stats_gen | baseline_32k | 10 | 10 | 100% [72%, 100%] | 50.3 [39.3, 75.2] |
| coding_05_scientific_lan | baseline_32k | 10 | 9 | 90% [60%, 98%] | 12.8 [9.8, 14.1] |
| coding_06_tax_rates_lan | baseline_32k | 10 | 3 | 30% [11%, 60%] | 13.3 [12.0, 14.6] |
| comms_07_confirm_send | baseline_32k | 10 | 10 | 100% [72%, 100%] | 90.1 [85.3, 121.3] |
| logtriage_04_pool_exhaustion | baseline_32k | 10 | 8 | 80% [49%, 94%] | 133.1 [98.5, 152.4] |
| longctx_06_tidal_report | baseline_32k | 10 | 10 | 100% [72%, 100%] | 62.3 [59.8, 65.2] |
| refactor_02_rename_price | baseline_32k | 10 | 9 | 90% [60%, 98%] | 80.7 [59.1, 106.2] |
| repoqa_05_retry_policy | baseline_32k | 10 | 5 | 50% [24%, 76%] | 28.1 [25.2, 36.5] |
| research_08e_free_will | baseline_32k | 10 | 3 | 30% [11%, 60%] | 220.7 [99.8, 265.6] |

\* directional: fewer than 5 trials

## Where the time goes

### baseline_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 10 | 359 | 36.3 | 58.2 | 0.0 | 5.5 | 0.0 |
| code_gen | 40 | 6311 | 16.2 | 76.8 | 5.7 | 1.3 | 0.0 |
| code_review | 10 | 473 | 11.1 | 85.1 | 0.0 | 3.9 | 0.0 |
| comms | 10 | 1021 | 31.3 | 55.4 | 0.0 | 13.2 | 0.0 |
| log_triage | 10 | 1429 | 63.6 | 34.8 | 0.0 | 1.5 | 0.0 |
| long_context | 10 | 631 | 70.9 | 26.2 | 0.0 | 3.0 | 0.0 |
| reasoning | 20 | 283 | 28.0 | 59.4 | 0.0 | 12.6 | 0.0 |
| refactor | 10 | 796 | 40.0 | 57.5 | 0.0 | 2.5 | 0.0 |
| repo_qa | 10 | 304 | 35.1 | 59.0 | 0.0 | 5.9 | 0.0 |
| research | 10 | 1957 | 56.7 | 36.9 | 4.8 | 1.6 | 0.0 |
| all tasks | 140 | 13565 | 33.1 | 60.5 | 3.4 | 3.0 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 33% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

## Instrumentation parity

Verdict: **inconclusive** (28 uninstrumented trials). See `parity.md`.

