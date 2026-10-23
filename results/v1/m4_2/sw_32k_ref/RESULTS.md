# Results: sw_32k_ref

Run seed 1, n = 3-5 per cell (varies by task), configurations: sw_32k (baseline: **sw_32k**). 80 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 80 | 72% [62%, 81%] | 2 | 0 | 2673.0 [1597.8, 3980.8] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_32k | 5 | 5 | 100% [57%, 100%] | 62.0 [54.3, 68.1] |
| bugfix_09_booking_slots | sw_32k | 5 | 2 | 40% [12%, 77%] | 149.2 [90.0, 163.2] |
| c_ringbuf_03_ringbuffer | sw_32k | 3* | 2 | 67% [21%, 94%] | 528.5 [155.9, 564.1] |
| coding_01_my_sqrt_review | sw_32k | 5 | 4 | 80% [38%, 96%] | 73.6 [69.4, 75.5] |
| coding_02_fizzbuzz_gen | sw_32k | 5 | 5 | 100% [57%, 100%] | 65.9 [52.0, 83.6] |
| coding_03_compress_gen | sw_32k | 5 | 5 | 100% [57%, 100%] | 59.2 [51.6, 70.7] |
| coding_04_dir_stats_gen | sw_32k | 5 | 4 | 80% [38%, 96%] | 70.5 [48.7, 92.4] |
| coding_05_scientific_lan | sw_32k | 5 | 5 | 100% [57%, 100%] | 40.8 [39.1, 45.5] |
| coding_06_tax_rates_lan | sw_32k | 5 | 1 | 20% [4%, 62%] | 41.0 [39.8, 45.6] |
| coding_10_wcx_constrained | sw_32k | 3* | 1 | 33% [6%, 79%] | 280.9 [66.2, 600.0] |
| comms_07_confirm_send | sw_32k | 5 | 5 | 100% [57%, 100%] | 124.3 [101.7, 128.5] |
| comms_12_timezone_slot | sw_32k | 5 | 4 | 80% [38%, 96%] | 224.6 [202.1, 900.1] |
| logtriage_04_pool_exhaustion | sw_32k | 5 | 4 | 80% [38%, 96%] | 127.9 [125.7, 185.0] |
| longctx_06_tidal_report | sw_32k | 3* | 3 | 100% [44%, 100%] | 91.5 [91.4, 92.5] |
| longctx_11_harbor_report | sw_32k | 3* | 2 | 67% [21%, 94%] | 199.3 [125.6, 264.4] |
| refactor_02_rename_price | sw_32k | 5 | 3 | 60% [23%, 88%] | 110.3 [54.2, 121.9] |
| repoqa_05_retry_policy | sw_32k | 5 | 3 | 60% [23%, 88%] | 53.9 [49.0, 60.0] |
| research_08e_free_will | sw_32k | 3* | 0 | 0% [0%, 56%] | 369.6 [181.1, 419.7] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 10 | 984 | 52.3 | 48.4 | 0.0 | 9.8 | -10.5 |
| code_gen | 21 | 3186 | 30.4 | 56.6 | 9.3 | 3.7 | 0.0 |
| code_review | 5 | 364 | 45.0 | 52.5 | 0.0 | 2.5 | 0.0 |
| comms | 10 | 2353 | 18.5 | 78.0 | 0.0 | 3.5 | 0.0 |
| log_triage | 5 | 699 | 61.8 | 36.6 | 0.0 | 1.6 | 0.0 |
| long_context | 6 | 865 | 58.0 | 40.7 | 0.0 | 1.3 | 0.0 |
| reasoning | 10 | 415 | 73.7 | 22.0 | 0.0 | 4.3 | 0.0 |
| refactor | 5 | 509 | 53.9 | 44.1 | 0.0 | 2.0 | 0.0 |
| repo_qa | 5 | 274 | 69.0 | 27.6 | 0.0 | 3.3 | 0.0 |
| research | 3 | 970 | 69.9 | 28.3 | 0.9 | 0.9 | 0.0 |
| all tasks | 80 | 10620 | 42.1 | 52.5 | 2.9 | 3.5 | -1.0 |

NVML GPU activity (any kernel per sample window): 95% of wall-clock; first LLM call of a session = 67% of prefill time; 3 trial(s) flagged by the decomposition checks.

## Comparison with the baseline

Only one configuration: nothing to compare.

