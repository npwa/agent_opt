# Results: q3_8b_core_n3

Run seed 1, n = 3 per cell, configurations: sw_q3_8b_32k_kvq8 (baseline: **sw_q3_8b_32k_kvq8**). 42 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_q3_8b_32k_kvq8 | 42 | 24% [13%, 39%] | 1 | 0 | 1402.7 [686.5, 2446.3] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_09_booking_slots | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 638.4 [124.5, 638.8] |
| c_ringbuf_03_ringbuffer | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 37.0 [22.3, 166.9] |
| coding_01_my_sqrt_review | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 18.1 [16.2, 19.1] |
| coding_02_fizzbuzz_gen | sw_q3_8b_32k_kvq8 | 3* | 2 | 67% [21%, 94%] | 35.1 [15.5, 57.6] |
| coding_03_compress_gen | sw_q3_8b_32k_kvq8 | 3* | 1 | 33% [6%, 79%] | 80.0 [47.5, 102.7] |
| coding_05_scientific_lan | sw_q3_8b_32k_kvq8 | 3* | 1 | 33% [6%, 79%] | 12.5 [7.5, 20.9] |
| coding_06_tax_rates_lan | sw_q3_8b_32k_kvq8 | 3* | 3 | 100% [44%, 100%] | 7.1 [7.0, 7.9] |
| coding_10_wcx_constrained | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 146.6 [110.2, 275.4] |
| comms_12_timezone_slot | sw_q3_8b_32k_kvq8 | 3* | 1 | 33% [6%, 79%] | 254.7 [207.5, 900.0] |
| logtriage_04_pool_exhaustion | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 35.6 [20.5, 64.0] |
| longctx_11_harbor_report | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 23.7 [23.1, 35.5] |
| refactor_02_rename_price | sw_q3_8b_32k_kvq8 | 3* | 2 | 67% [21%, 94%] | 37.5 [33.2, 67.5] |
| repoqa_05_retry_policy | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 28.6 [19.3, 35.7] |
| research_08e_free_will | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 47.8 [32.2, 54.3] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_q3_8b_32k_kvq8

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 1402 | 2.0 | 97.4 | 0.0 | 0.6 | 0.0 |
| code_gen | 12 | 1097 | 3.7 | 94.2 | 0.0 | 2.1 | 0.0 |
| code_review | 3 | 53 | 13.5 | 76.0 | 0.0 | 10.4 | 0.0 |
| comms | 3 | 1362 | 1.0 | 98.4 | 0.0 | 0.6 | 0.0 |
| log_triage | 3 | 121 | 11.0 | 80.3 | 3.2 | 5.4 | 0.0 |
| long_context | 3 | 82 | 26.2 | 67.0 | 0.0 | 6.8 | 0.0 |
| reasoning | 6 | 63 | 22.0 | 58.6 | 2.2 | 17.2 | 0.0 |
| refactor | 3 | 138 | 8.1 | 59.2 | 19.1 | 13.6 | 0.0 |
| repo_qa | 3 | 84 | 8.0 | 85.3 | 0.0 | 6.8 | 0.0 |
| research | 3 | 134 | 6.4 | 77.3 | 9.9 | 6.3 | 0.0 |
| all tasks | 42 | 4537 | 3.6 | 93.2 | 1.0 | 2.2 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 61% of prefill time; 1 trial(s) flagged by the decomposition checks.

## Comparison with the baseline

Only one configuration: nothing to compare.

