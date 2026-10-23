# Results: sw_16k

Run seed 1, n = 3-10 per cell (varies by task), configurations: sw_16k (baseline: **sw_16k**). 159 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_16k | 159 | 65% [58%, 72%] | 0 | 0 | 2008.4 [1518.0, 2579.0] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| bugfix_01_moving_average | sw_16k | 10 | 10 | 100% [72%, 100%] | 61.0 [55.8, 64.6] |
| bugfix_09_booking_slots | sw_16k | 10 | 4 | 40% [17%, 69%] | 124.4 [111.5, 180.1] |
| c_ringbuf_03_ringbuffer | sw_16k | 10 | 6 | 60% [31%, 83%] | 279.4 [126.7, 348.9] |
| coding_01_my_sqrt_review | sw_16k | 10 | 6 | 60% [31%, 83%] | 63.9 [54.5, 77.7] |
| coding_02_fizzbuzz_gen | sw_16k | 10 | 10 | 100% [72%, 100%] | 58.3 [51.9, 68.2] |
| coding_03_compress_gen | sw_16k | 10 | 8 | 80% [49%, 94%] | 65.0 [57.9, 82.4] |
| coding_04_dir_stats_gen | sw_16k | 10 | 10 | 100% [72%, 100%] | 71.6 [59.4, 87.0] |
| coding_05_scientific_lan | sw_16k | 10 | 10 | 100% [72%, 100%] | 40.2 [38.6, 41.4] |
| coding_06_tax_rates_lan | sw_16k | 10 | 3 | 30% [11%, 60%] | 40.5 [39.5, 41.5] |
| coding_10_wcx_constrained | sw_16k | 10 | 4 | 40% [17%, 69%] | 264.6 [141.0, 353.2] |
| comms_07_confirm_send | sw_16k | 10 | 10 | 100% [72%, 100%] | 105.6 [103.8, 113.7] |
| comms_12_timezone_slot | sw_16k | 10 | 5 | 50% [24%, 76%] | 175.3 [153.3, 213.0] |
| logtriage_04_pool_exhaustion | sw_16k | 10 | 1 | 10% [2%, 40%] | 137.8 [106.5, 164.0] |
| longctx_06_tidal_report | sw_16k | 3* | 0 | 0% [0%, 56%] | 117.9 [113.1, 136.3] |
| longctx_11_harbor_report | sw_16k | 3* | 0 | 0% [0%, 56%] | 103.5 [95.1, 160.4] |
| refactor_02_rename_price | sw_16k | 10 | 9 | 90% [60%, 98%] | 98.8 [63.2, 121.6] |
| repoqa_05_retry_policy | sw_16k | 10 | 7 | 70% [40%, 89%] | 55.6 [49.8, 58.5] |
| research_08e_free_will | sw_16k | 3* | 1 | 33% [6%, 79%] | 144.8 [96.4, 266.4] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_16k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 20 | 2020 | 50.0 | 48.0 | 0.0 | 2.0 | 0.0 |
| code_gen | 50 | 7411 | 33.3 | 65.3 | 0.0 | 1.4 | 0.0 |
| code_review | 10 | 678 | 47.4 | 49.8 | 0.0 | 2.8 | 0.0 |
| comms | 20 | 3128 | 25.9 | 68.8 | 0.0 | 5.3 | 0.0 |
| log_triage | 10 | 1394 | 61.6 | 36.8 | 0.0 | 1.6 | 0.0 |
| long_context | 6 | 726 | 43.6 | 54.8 | 0.0 | 1.6 | 0.0 |
| reasoning | 20 | 806 | 73.9 | 21.6 | 0.0 | 4.5 | 0.0 |
| refactor | 10 | 951 | 55.5 | 42.4 | 0.0 | 2.2 | 0.0 |
| repo_qa | 10 | 551 | 67.5 | 29.1 | 0.0 | 3.4 | 0.0 |
| research | 3 | 508 | 64.5 | 31.7 | 1.9 | 1.9 | 0.0 |
| all tasks | 159 | 18173 | 41.9 | 55.6 | 0.1 | 2.5 | 0.0 |

NVML GPU activity (any kernel per sample window): 95% of wall-clock; first LLM call of a session = 70% of prefill time.

## Comparison with the baseline

Only one configuration: nothing to compare.

