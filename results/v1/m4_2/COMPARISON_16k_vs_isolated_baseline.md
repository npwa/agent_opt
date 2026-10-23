# Comparison: baseline_isolated (A) vs sw_16k (B)

14 common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), percentile bootstrap (times); the number of trials per task is in the pass counts (A pass, B pass).

## Success

- Overall: A 99/140 = 71 % [63 %, 78 %]; B 91/126 = 72 % [64 %, 79 %]; difference +1.5 pp [-9.4, +12.2].

| Task | A pass | B pass | difference (95 % CI), pp |
|---|---:|---:|---|
| bugfix_01_moving_average | 10/10 | 10/10 | +0 [-28, +28] |
| c_ringbuf_03_ringbuffer | 3/10 | 6/10 | +30 [-12, +60] |
| coding_01_my_sqrt_review | 5/10 | 6/10 | +10 [-29, +45] |
| coding_02_fizzbuzz_gen | 8/10 | 10/10 | +20 [-11, +51] |
| coding_03_compress_gen | 6/10 | 8/10 | +20 [-19, +52] |
| coding_04_dir_stats_gen | 10/10 | 10/10 | +0 [-28, +28] |
| coding_05_scientific_lan | 9/10 | 10/10 | +10 [-19, +40] |
| coding_06_tax_rates_lan | 3/10 | 3/10 | +0 [-36, +36] |
| comms_07_confirm_send | 10/10 | 10/10 | +0 [-28, +28] |
| logtriage_04_pool_exhaustion | 8/10 | 1/10 | -70 [-87, -27] |
| longctx_06_tidal_report | 10/10 | 0/3 | -100 [-100, -37] |
| refactor_02_rename_price | 9/10 | 9/10 | +0 [-32, +32] |
| repoqa_05_retry_policy | 5/10 | 7/10 | +20 [-20, +53] |
| research_08e_free_will | 3/10 | 1/3 | +3 [-37, +53] |

## Time per trial

| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |
|---|---:|---:|---|---:|
| bugfix_01_moving_average | 33.5 | 61.0 | 1.82 [1.43, 2.04] | 0.000183 |
| c_ringbuf_03_ringbuffer | 401.1 | 279.4 | 0.70 [0.28, 1.90] | 0.186 |
| coding_01_my_sqrt_review | 43.7 | 63.9 | 1.46 [1.19, 2.11] | 0.00911 |
| coding_02_fizzbuzz_gen | 24.4 | 58.3 | 2.40 [1.33, 3.52] | 0.00283 |
| coding_03_compress_gen | 45.0 | 65.0 | 1.44 [0.76, 2.56] | 0.14 |
| coding_04_dir_stats_gen | 50.3 | 71.6 | 1.42 [0.95, 1.87] | 0.0311 |
| coding_05_scientific_lan | 12.8 | 40.2 | 3.14 [2.84, 4.13] | 0.000183 |
| coding_06_tax_rates_lan | 13.3 | 40.5 | 3.04 [2.77, 3.38] | 0.00169 |
| comms_07_confirm_send | 90.1 | 105.6 | 1.17 [0.92, 1.29] | 0.212 |
| logtriage_04_pool_exhaustion | 133.1 | 137.8 | 1.04 [0.78, 1.56] | 0.571 |
| longctx_06_tidal_report | 62.3 | 117.9 | 1.89 [1.76, 2.24] | 0.00699 |
| refactor_02_rename_price | 80.7 | 98.8 | 1.22 [0.68, 1.74] | 0.427 |
| repoqa_05_retry_policy | 28.1 | 55.6 | 1.98 [1.46, 2.18] | 0.000182 |
| research_08e_free_will | 220.7 | 144.8 | 0.66 [0.37, 2.12] | 0.937 |

- Pooled, normalised by each task's A median: median ratio B/A = 1.57 [1.42, 1.82] (n = 140 vs 126, Mann-Whitney p = 3.3e-14).
- Mean time per trial: A 97 s, B 93 s (-4 %). Suite time (sum of task medians): A 1239 s, B 2008 s.

## Where the time goes (all tasks, share of wall-clock)

| Bucket | A | B |
|---|---:|---:|
| prefill | 33.1 % | 41.9 % |
| decode | 60.5 % | 55.6 % |
| tool | 3.4 % | 0.1 % |
| harness | 3.0 % | 2.5 % |
| unattributed | 0.0 % | -0.0 % |
| wall-clock per trial | 97 s | 114 s |

## Prompt and cache

| | A | B |
|---|---:|---:|
| first-call prompt tokens (median) | 7326 | 7340 |
| cold prefill rate (tok/s) | 257 | 268 |
| first-call ttft, median (s) | 2.7 | 29.7 |
| first calls cold / warm | 10 / 130 | 159 / 0 |
| first calls, share of prefill time | 15 % | 64 % |

### Order dependence in B (first call of a trial vs the previous trial, 158 pairs)

| previous trial's first prompt | first call cold | first call warm |
|---|---:|---:|
| same size (within 30 tokens) | 66 | 0 |
| different size | 92 | 0 |

Cold first calls (task, previous task): coding_05_scientific_lan after comms_12_timezone_slot; comms_12_timezone_slot after coding_05_scientific_lan; refactor_02_rename_price after comms_12_timezone_slot; coding_06_tax_rates_lan after refactor_02_rename_price; bugfix_01_moving_average after coding_06_tax_rates_lan; longctx_11_harbor_report after bugfix_01_moving_average; comms_12_timezone_slot after longctx_11_harbor_report; coding_05_scientific_lan after comms_12_timezone_slot; bugfix_09_booking_slots after coding_05_scientific_lan; coding_10_wcx_constrained after bugfix_09_booking_slots; coding_02_fizzbuzz_gen after coding_10_wcx_constrained; coding_02_fizzbuzz_gen after coding_02_fizzbuzz_gen; bugfix_01_moving_average after coding_02_fizzbuzz_gen; longctx_06_tidal_report after bugfix_01_moving_average; research_08e_free_will after longctx_06_tidal_report; coding_04_dir_stats_gen after research_08e_free_will; longctx_06_tidal_report after coding_04_dir_stats_gen; comms_07_confirm_send after longctx_06_tidal_report; coding_03_compress_gen after comms_07_confirm_send; bugfix_09_booking_slots after coding_03_compress_gen; coding_04_dir_stats_gen after bugfix_09_booking_slots; bugfix_09_booking_slots after coding_04_dir_stats_gen; coding_02_fizzbuzz_gen after bugfix_09_booking_slots; coding_02_fizzbuzz_gen after coding_02_fizzbuzz_gen; repoqa_05_retry_policy after coding_02_fizzbuzz_gen; logtriage_04_pool_exhaustion after repoqa_05_retry_policy; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; refactor_02_rename_price after coding_02_fizzbuzz_gen; coding_06_tax_rates_lan after refactor_02_rename_price; repoqa_05_retry_policy after coding_06_tax_rates_lan; refactor_02_rename_price after repoqa_05_retry_policy; coding_05_scientific_lan after refactor_02_rename_price; coding_03_compress_gen after coding_05_scientific_lan; comms_12_timezone_slot after coding_03_compress_gen; repoqa_05_retry_policy after comms_12_timezone_slot; refactor_02_rename_price after repoqa_05_retry_policy; c_ringbuf_03_ringbuffer after refactor_02_rename_price; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; coding_06_tax_rates_lan after coding_05_scientific_lan; coding_01_my_sqrt_review after coding_06_tax_rates_lan; bugfix_01_moving_average after coding_01_my_sqrt_review; logtriage_04_pool_exhaustion after bugfix_01_moving_average; coding_10_wcx_constrained after logtriage_04_pool_exhaustion; coding_06_tax_rates_lan after coding_10_wcx_constrained; repoqa_05_retry_policy after coding_06_tax_rates_lan; coding_05_scientific_lan after repoqa_05_retry_policy; comms_07_confirm_send after coding_05_scientific_lan; bugfix_09_booking_slots after comms_07_confirm_send; coding_10_wcx_constrained after bugfix_09_booking_slots; coding_03_compress_gen after coding_10_wcx_constrained; coding_10_wcx_constrained after coding_03_compress_gen; coding_06_tax_rates_lan after coding_10_wcx_constrained; coding_01_my_sqrt_review after coding_06_tax_rates_lan; c_ringbuf_03_ringbuffer after coding_01_my_sqrt_review; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; coding_01_my_sqrt_review after bugfix_09_booking_slots; longctx_06_tidal_report after coding_01_my_sqrt_review; research_08e_free_will after longctx_06_tidal_report; logtriage_04_pool_exhaustion after research_08e_free_will; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; refactor_02_rename_price after coding_02_fizzbuzz_gen; coding_05_scientific_lan after refactor_02_rename_price; logtriage_04_pool_exhaustion after coding_05_scientific_lan; coding_10_wcx_constrained after logtriage_04_pool_exhaustion; comms_07_confirm_send after coding_10_wcx_constrained; c_ringbuf_03_ringbuffer after comms_07_confirm_send; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; coding_01_my_sqrt_review after coding_05_scientific_lan; refactor_02_rename_price after coding_01_my_sqrt_review; coding_04_dir_stats_gen after refactor_02_rename_price; bugfix_01_moving_average after coding_04_dir_stats_gen; coding_04_dir_stats_gen after bugfix_01_moving_average; refactor_02_rename_price after coding_04_dir_stats_gen; comms_07_confirm_send after refactor_02_rename_price; coding_04_dir_stats_gen after comms_07_confirm_send; c_ringbuf_03_ringbuffer after coding_04_dir_stats_gen; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; repoqa_05_retry_policy after bugfix_09_booking_slots; logtriage_04_pool_exhaustion after repoqa_05_retry_policy; bugfix_09_booking_slots after logtriage_04_pool_exhaustion; coding_02_fizzbuzz_gen after bugfix_09_booking_slots; logtriage_04_pool_exhaustion after coding_02_fizzbuzz_gen; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; c_ringbuf_03_ringbuffer after coding_02_fizzbuzz_gen; research_08e_free_will after c_ringbuf_03_ringbuffer; comms_12_timezone_slot after research_08e_free_will; coding_03_compress_gen after comms_12_timezone_slot; coding_01_my_sqrt_review after coding_03_compress_gen; coding_04_dir_stats_gen after coding_01_my_sqrt_review; bugfix_01_moving_average after coding_04_dir_stats_gen; comms_07_confirm_send after bugfix_01_moving_average; coding_03_compress_gen after comms_07_confirm_send; comms_07_confirm_send after coding_03_compress_gen; coding_04_dir_stats_gen after comms_07_confirm_send; coding_05_scientific_lan after coding_04_dir_stats_gen; coding_01_my_sqrt_review after coding_05_scientific_lan; coding_01_my_sqrt_review after coding_01_my_sqrt_review; repoqa_05_retry_policy after coding_01_my_sqrt_review; coding_06_tax_rates_lan after repoqa_05_retry_policy; comms_07_confirm_send after coding_06_tax_rates_lan; coding_03_compress_gen after comms_07_confirm_send; coding_04_dir_stats_gen after coding_03_compress_gen; coding_10_wcx_constrained after coding_04_dir_stats_gen; comms_07_confirm_send after coding_10_wcx_constrained; coding_02_fizzbuzz_gen after comms_07_confirm_send; coding_10_wcx_constrained after coding_02_fizzbuzz_gen; bugfix_09_booking_slots after coding_10_wcx_constrained; comms_12_timezone_slot after bugfix_09_booking_slots; coding_10_wcx_constrained after comms_12_timezone_slot; coding_06_tax_rates_lan after coding_10_wcx_constrained; c_ringbuf_03_ringbuffer after coding_06_tax_rates_lan; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; coding_06_tax_rates_lan after bugfix_09_booking_slots; coding_05_scientific_lan after coding_06_tax_rates_lan; comms_07_confirm_send after coding_05_scientific_lan; repoqa_05_retry_policy after comms_07_confirm_send; longctx_11_harbor_report after repoqa_05_retry_policy; coding_01_my_sqrt_review after longctx_11_harbor_report; refactor_02_rename_price after coding_01_my_sqrt_review; longctx_11_harbor_report after refactor_02_rename_price; c_ringbuf_03_ringbuffer after longctx_11_harbor_report; coding_06_tax_rates_lan after c_ringbuf_03_ringbuffer; logtriage_04_pool_exhaustion after coding_06_tax_rates_lan; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; c_ringbuf_03_ringbuffer after coding_02_fizzbuzz_gen; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; repoqa_05_retry_policy after coding_05_scientific_lan; comms_12_timezone_slot after repoqa_05_retry_policy; coding_03_compress_gen after comms_12_timezone_slot; refactor_02_rename_price after coding_03_compress_gen; comms_07_confirm_send after refactor_02_rename_price; coding_03_compress_gen after comms_07_confirm_send; repoqa_05_retry_policy after coding_03_compress_gen; bugfix_01_moving_average after repoqa_05_retry_policy; bugfix_01_moving_average after bugfix_01_moving_average; bugfix_01_moving_average after bugfix_01_moving_average; repoqa_05_retry_policy after bugfix_01_moving_average; coding_06_tax_rates_lan after repoqa_05_retry_policy; c_ringbuf_03_ringbuffer after coding_06_tax_rates_lan; coding_03_compress_gen after c_ringbuf_03_ringbuffer; coding_04_dir_stats_gen after coding_03_compress_gen; comms_12_timezone_slot after coding_04_dir_stats_gen; bugfix_01_moving_average after comms_12_timezone_slot; comms_12_timezone_slot after bugfix_01_moving_average; coding_10_wcx_constrained after comms_12_timezone_slot; bugfix_01_moving_average after coding_10_wcx_constrained; logtriage_04_pool_exhaustion after bugfix_01_moving_average; c_ringbuf_03_ringbuffer after logtriage_04_pool_exhaustion; coding_03_compress_gen after c_ringbuf_03_ringbuffer; coding_10_wcx_constrained after coding_03_compress_gen; logtriage_04_pool_exhaustion after coding_10_wcx_constrained; comms_12_timezone_slot after logtriage_04_pool_exhaustion; logtriage_04_pool_exhaustion after comms_12_timezone_slot; coding_01_my_sqrt_review after logtriage_04_pool_exhaustion; coding_04_dir_stats_gen after coding_01_my_sqrt_review; bugfix_09_booking_slots after coding_04_dir_stats_gen; refactor_02_rename_price after bugfix_09_booking_slots; coding_01_my_sqrt_review after refactor_02_rename_price.
