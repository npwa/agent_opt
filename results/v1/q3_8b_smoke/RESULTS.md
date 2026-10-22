# Results: q3_8b_smoke

Run seed 1, n = 1 per cell, configurations: sw_q3_8b_32k_kvq8 (baseline: **sw_q3_8b_32k_kvq8**). 18 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_q3_8b_32k_kvq8 | 18 | 17% [6%, 39%] | 0 | 0 | 1629.5 [1629.5, 1629.5] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_q3_8b_32k_kvq8 | 1* | 1 | 100% [21%, 100%] | 43.0 [43.0, 43.0] |
| bugfix_09_booking_slots | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 638.3 [638.3, 638.3] |
| c_ringbuf_03_ringbuffer | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 23.7 [23.7, 23.7] |
| coding_01_my_sqrt_review | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 20.6 [20.6, 20.6] |
| coding_02_fizzbuzz_gen | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 26.2 [26.2, 26.2] |
| coding_03_compress_gen | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 56.7 [56.7, 56.7] |
| coding_04_dir_stats_gen | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 36.0 [36.0, 36.0] |
| coding_05_scientific_lan | sw_q3_8b_32k_kvq8 | 1* | 1 | 100% [21%, 100%] | 27.1 [27.1, 27.1] |
| coding_06_tax_rates_lan | sw_q3_8b_32k_kvq8 | 1* | 1 | 100% [21%, 100%] | 7.1 [7.1, 7.1] |
| coding_10_wcx_constrained | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 34.9 [34.9, 34.9] |
| comms_07_confirm_send | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 63.1 [63.1, 63.1] |
| comms_12_timezone_slot | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 256.1 [256.1, 256.1] |
| logtriage_04_pool_exhaustion | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 21.5 [21.5, 21.5] |
| longctx_06_tidal_report | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 15.7 [15.7, 15.7] |
| longctx_11_harbor_report | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 58.3 [58.3, 58.3] |
| refactor_02_rename_price | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 229.0 [229.0, 229.0] |
| repoqa_05_retry_policy | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 35.4 [35.4, 35.4] |
| research_08e_free_will | sw_q3_8b_32k_kvq8 | 1* | 0 | 0% [0%, 79%] | 36.8 [36.8, 36.8] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_q3_8b_32k_kvq8

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 2 | 681 | 2.3 | 97.0 | 0.0 | 0.8 | 0.0 |
| code_gen | 5 | 177 | 8.2 | 86.6 | 0.0 | 5.2 | 0.0 |
| code_review | 1 | 21 | 13.0 | 78.0 | 0.0 | 9.0 | 0.0 |
| comms | 2 | 319 | 2.0 | 92.9 | 0.0 | 5.1 | 0.0 |
| log_triage | 1 | 22 | 9.6 | 80.8 | 0.0 | 9.6 | 0.0 |
| long_context | 2 | 74 | 12.7 | 82.7 | 0.0 | 4.6 | 0.0 |
| reasoning | 2 | 34 | 14.9 | 51.9 | 23.3 | 10.0 | 0.0 |
| refactor | 1 | 229 | 2.7 | 36.9 | 51.0 | 9.3 | 0.0 |
| repo_qa | 1 | 35 | 6.3 | 88.3 | 0.0 | 5.4 | 0.0 |
| research | 1 | 37 | 7.0 | 76.8 | 7.2 | 9.0 | 0.0 |
| all tasks | 18 | 1630 | 4.1 | 83.9 | 7.8 | 4.2 | 0.0 |

NVML GPU activity (any kernel per sample window): 85% of wall-clock; first LLM call of a session = 65% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

