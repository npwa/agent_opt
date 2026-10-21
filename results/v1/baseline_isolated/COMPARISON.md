# Comparison: baseline (A) vs baseline_isolated (B)

14 common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), percentile bootstrap (times); n = 10 per task.

## Success

- Overall: A 104/140 = 74 % [66 %, 81 %]; B 99/140 = 71 % [63 %, 78 %]; difference -3.6 pp [-13.9, +6.9].

| Task | A pass | B pass | difference (95 % CI), pp |
|---|---:|---:|---|
| bugfix_01_moving_average | 8/10 | 10/10 | +20 [-11, +51] |
| c_ringbuf_03_ringbuffer | 6/10 | 3/10 | -30 [-60, +12] |
| coding_01_my_sqrt_review | 6/10 | 5/10 | -10 [-45, +29] |
| coding_02_fizzbuzz_gen | 8/10 | 8/10 | +0 [-34, +34] |
| coding_03_compress_gen | 8/10 | 6/10 | -20 [-52, +19] |
| coding_04_dir_stats_gen | 10/10 | 10/10 | +0 [-28, +28] |
| coding_05_scientific_lan | 10/10 | 9/10 | -10 [-40, +19] |
| coding_06_tax_rates_lan | 3/10 | 3/10 | +0 [-36, +36] |
| comms_07_confirm_send | 10/10 | 10/10 | +0 [-28, +28] |
| logtriage_04_pool_exhaustion | 8/10 | 8/10 | +0 [-34, +34] |
| longctx_06_tidal_report | 10/10 | 10/10 | +0 [-28, +28] |
| refactor_02_rename_price | 9/10 | 9/10 | +0 [-32, +32] |
| repoqa_05_retry_policy | 7/10 | 5/10 | -20 [-53, +20] |
| research_08e_free_will | 1/10 | 3/10 | +20 [-16, +51] |

## Time per trial

| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |
|---|---:|---:|---|---:|
| bugfix_01_moving_average | 72.0 | 33.5 | 0.47 [0.40, 0.60] | 0.000183 |
| c_ringbuf_03_ringbuffer | 351.5 | 401.1 | 1.14 [0.36, 2.86] | 0.85 |
| coding_01_my_sqrt_review | 77.3 | 43.7 | 0.56 [0.40, 0.68] | 0.00459 |
| coding_02_fizzbuzz_gen | 78.3 | 24.4 | 0.31 [0.22, 0.55] | 0.0022 |
| coding_03_compress_gen | 77.6 | 45.0 | 0.58 [0.33, 1.04] | 0.0452 |
| coding_04_dir_stats_gen | 81.2 | 50.3 | 0.62 [0.46, 0.93] | 0.0173 |
| coding_05_scientific_lan | 50.0 | 12.8 | 0.26 [0.19, 0.28] | 0.000182 |
| coding_06_tax_rates_lan | 50.9 | 13.3 | 0.26 [0.24, 0.29] | 0.000182 |
| comms_07_confirm_send | 111.7 | 90.1 | 0.81 [0.76, 1.08] | 0.0757 |
| logtriage_04_pool_exhaustion | 157.8 | 133.1 | 0.84 [0.61, 1.01] | 0.0757 |
| longctx_06_tidal_report | 101.4 | 62.3 | 0.61 [0.59, 0.65] | 0.000183 |
| refactor_02_rename_price | 111.2 | 80.7 | 0.73 [0.52, 1.02] | 0.0376 |
| repoqa_05_retry_policy | 67.2 | 28.1 | 0.42 [0.36, 0.54] | 0.000182 |
| research_08e_free_will | 208.2 | 220.7 | 1.06 [0.46, 1.60] | 0.521 |

- Pooled, normalised by each task's A median: median ratio B/A = 0.58 [0.52, 0.63] (n = 140 vs 140, Mann-Whitney p = 1.55e-20).
- Mean time per trial: A 123 s, B 97 s (-22 %). Suite time (sum of task medians): A 1596 s, B 1239 s.

## Where the time goes (all tasks, share of wall-clock)

| Bucket | A | B |
|---|---:|---:|
| prefill | 54.0 % | 33.1 % |
| decode | 42.7 % | 60.5 % |
| tool | 0.9 % | 3.4 % |
| harness | 2.4 % | 3.0 % |
| unattributed | 0.0 % | 0.0 % |
| wall-clock per trial | 123 s | 97 s |

## Prompt and cache

| | A | B |
|---|---:|---:|
| first-call prompt tokens (median) | 10142 | 7326 |
| cold prefill rate (tok/s) | 253 | 257 |
| first-call ttft, median (s) | 40.6 | 2.7 |
| first calls cold / warm | 140 / 0 | 10 / 130 |
| first calls, share of prefill time | 62 % | 15 % |

### Order dependence in B (first call of a trial vs the previous trial, 139 pairs)

| previous trial's first prompt | first call cold | first call warm |
|---|---:|---:|
| same size (within 30 tokens) | 0 | 73 |
| different size | 9 | 57 |

Cold first calls (task, previous task): coding_06_tax_rates_lan after comms_07_confirm_send; research_08e_free_will after c_ringbuf_03_ringbuffer; research_08e_free_will after coding_05_scientific_lan; comms_07_confirm_send after coding_06_tax_rates_lan; refactor_02_rename_price after comms_07_confirm_send; research_08e_free_will after coding_02_fizzbuzz_gen; research_08e_free_will after coding_06_tax_rates_lan; research_08e_free_will after coding_06_tax_rates_lan; comms_07_confirm_send after logtriage_04_pool_exhaustion.
