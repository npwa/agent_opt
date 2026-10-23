# Results: q3_8b_core_n3_confined

Run seed 1, n = 3 per cell, configurations: sw_q3_8b_32k_kvq8 (baseline: **sw_q3_8b_32k_kvq8**). 42 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_q3_8b_32k_kvq8 | 42 | 24% [13%, 39%] | 0 | 0 | 1146.6 [579.5, 1753.6] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_09_booking_slots | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 321.8 [49.5, 746.3] |
| c_ringbuf_03_ringbuffer | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 177.9 [32.0, 194.7] |
| coding_01_my_sqrt_review | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 19.4 [16.7, 24.0] |
| coding_02_fizzbuzz_gen | sw_q3_8b_32k_kvq8 | 3* | 2 | 67% [21%, 94%] | 24.3 [11.3, 42.1] |
| coding_03_compress_gen | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 47.2 [26.2, 62.6] |
| coding_05_scientific_lan | sw_q3_8b_32k_kvq8 | 3* | 3 | 100% [44%, 100%] | 24.1 [15.7, 24.6] |
| coding_06_tax_rates_lan | sw_q3_8b_32k_kvq8 | 3* | 3 | 100% [44%, 100%] | 7.9 [7.2, 8.3] |
| coding_10_wcx_constrained | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 165.9 [150.2, 210.5] |
| comms_12_timezone_slot | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 193.8 [140.0, 224.5] |
| logtriage_04_pool_exhaustion | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 45.0 [35.1, 65.9] |
| longctx_11_harbor_report | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 33.8 [26.1, 37.3] |
| refactor_02_rename_price | sw_q3_8b_32k_kvq8 | 3* | 2 | 67% [21%, 94%] | 25.0 [19.0, 26.9] |
| repoqa_05_retry_policy | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 20.4 [14.3, 30.2] |
| research_08e_free_will | sw_q3_8b_32k_kvq8 | 3* | 0 | 0% [0%, 56%] | 40.1 [36.2, 55.7] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_q3_8b_32k_kvq8

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 3 | 1118 | 2.2 | 97.1 | 0.0 | 0.6 | 0.0 |
| code_gen | 12 | 1145 | 3.7 | 73.2 | 21.0 | 2.1 | 0.0 |
| code_review | 3 | 60 | 12.2 | 78.3 | 0.0 | 9.5 | 0.0 |
| comms | 3 | 558 | 1.7 | 96.8 | 0.0 | 1.4 | 0.0 |
| log_triage | 3 | 147 | 11.6 | 83.5 | 0.0 | 4.9 | 0.0 |
| long_context | 3 | 97 | 22.1 | 71.8 | 0.0 | 6.1 | 0.0 |
| reasoning | 6 | 88 | 16.9 | 59.8 | 10.4 | 12.8 | 0.0 |
| refactor | 3 | 71 | 10.9 | 80.7 | 0.0 | 8.4 | 0.0 |
| repo_qa | 3 | 65 | 10.5 | 80.5 | 0.0 | 8.9 | 0.0 |
| research | 3 | 132 | 6.4 | 70.1 | 16.4 | 7.2 | 0.0 |
| all tasks | 42 | 3481 | 4.6 | 85.0 | 7.8 | 2.6 | 0.0 |

NVML GPU activity (any kernel per sample window): 85% of wall-clock; first LLM call of a session = 62% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

