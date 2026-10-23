# Comparison: baseline_isolated (A) vs sw_32k_ref (B)

14 common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), percentile bootstrap (times); the number of trials per task is in the pass counts (A pass, B pass).

## Success

- Overall: A 99/140 = 71 % [63 %, 78 %]; B 49/64 = 77 % [65 %, 85 %]; difference +5.8 pp [-7.7, +17.7].

| Task | A pass | B pass | difference (95 % CI), pp |
|---|---:|---:|---|
| bugfix_01_moving_average | 10/10 | 5/5 | +0 [-43, +28] |
| c_ringbuf_03_ringbuffer | 3/10 | 2/3 | +37 [-18, +70] |
| coding_01_my_sqrt_review | 5/10 | 4/5 | +30 [-20, +61] |
| coding_02_fizzbuzz_gen | 8/10 | 5/5 | +20 [-26, +51] |
| coding_03_compress_gen | 6/10 | 5/5 | +40 [-9, +69] |
| coding_04_dir_stats_gen | 10/10 | 4/5 | -20 [-62, +12] |
| coding_05_scientific_lan | 9/10 | 5/5 | +10 [-34, +40] |
| coding_06_tax_rates_lan | 3/10 | 1/5 | -10 [-44, +37] |
| comms_07_confirm_send | 10/10 | 5/5 | +0 [-43, +28] |
| logtriage_04_pool_exhaustion | 8/10 | 4/5 | +0 [-45, +35] |
| longctx_06_tidal_report | 10/10 | 3/3 | +0 [-56, +28] |
| refactor_02_rename_price | 9/10 | 3/5 | -30 [-68, +12] |
| repoqa_05_retry_policy | 5/10 | 3/5 | +10 [-35, +49] |
| research_08e_free_will | 3/10 | 0/3 | -30 [-60, +29] |

## Time per trial

| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |
|---|---:|---:|---|---:|
| bugfix_01_moving_average | 33.5 | 62.0 | 1.85 [1.39, 2.13] | 0.000666 |
| c_ringbuf_03_ringbuffer | 401.1 | 528.5 | 1.32 [0.23, 3.50] | 1 |
| coding_01_my_sqrt_review | 43.7 | 73.6 | 1.68 [1.41, 2.31] | 0.0168 |
| coding_02_fizzbuzz_gen | 24.4 | 65.9 | 2.71 [1.54, 4.12] | 0.0127 |
| coding_03_compress_gen | 45.0 | 59.2 | 1.32 [0.76, 2.39] | 0.31 |
| coding_04_dir_stats_gen | 50.3 | 70.5 | 1.40 [0.85, 2.01] | 0.31 |
| coding_05_scientific_lan | 12.8 | 40.8 | 3.19 [2.87, 4.16] | 0.00267 |
| coding_06_tax_rates_lan | 13.3 | 41.0 | 3.07 [2.78, 3.49] | 0.0084 |
| comms_07_confirm_send | 90.1 | 124.3 | 1.38 [1.00, 1.47] | 0.129 |
| logtriage_04_pool_exhaustion | 133.1 | 127.9 | 0.96 [0.84, 1.44] | 0.953 |
| longctx_06_tidal_report | 62.3 | 91.5 | 1.47 [1.40, 1.55] | 0.00699 |
| refactor_02_rename_price | 80.7 | 110.3 | 1.37 [0.63, 1.90] | 0.129 |
| repoqa_05_retry_policy | 28.1 | 53.9 | 1.92 [1.47, 2.26] | 0.000666 |
| research_08e_free_will | 220.7 | 369.6 | 1.68 [0.70, 3.70] | 0.161 |

- Pooled, normalised by each task's A median: median ratio B/A = 1.61 [1.44, 1.79] (n = 140 vs 64, Mann-Whitney p = 1.49e-13).
- Mean time per trial: A 97 s, B 104 s (+7 %). Suite time (sum of task medians): A 1239 s, B 2673 s.

## Where the time goes (all tasks, share of wall-clock)

| Bucket | A | B |
|---|---:|---:|
| prefill | 33.1 % | 42.1 % |
| decode | 60.5 % | 52.5 % |
| tool | 3.4 % | 2.9 % |
| harness | 3.0 % | 3.5 % |
| unattributed | 0.0 % | -1.0 % |
| wall-clock per trial | 97 s | 133 s |

## Prompt and cache

| | A | B |
|---|---:|---:|
| first-call prompt tokens (median) | 7326 | 7342 |
| cold prefill rate (tok/s) | 257 | 250 |
| first-call ttft, median (s) | 2.7 | 30.4 |
| first calls cold / warm | 10 / 130 | 80 / 0 |
| first calls, share of prefill time | 15 % | 56 % |

### Order dependence in B (first call of a trial vs the previous trial, 79 pairs)

| previous trial's first prompt | first call cold | first call warm |
|---|---:|---:|
| same size (within 30 tokens) | 39 | 0 |
| different size | 40 | 0 |

Cold first calls (task, previous task): bugfix_01_moving_average after longctx_11_harbor_report; c_ringbuf_03_ringbuffer after bugfix_01_moving_average; bugfix_01_moving_average after c_ringbuf_03_ringbuffer; coding_10_wcx_constrained after bugfix_01_moving_average; coding_04_dir_stats_gen after coding_10_wcx_constrained; coding_05_scientific_lan after coding_04_dir_stats_gen; refactor_02_rename_price after coding_05_scientific_lan; coding_03_compress_gen after refactor_02_rename_price; bugfix_09_booking_slots after coding_03_compress_gen; comms_07_confirm_send after bugfix_09_booking_slots; coding_02_fizzbuzz_gen after comms_07_confirm_send; research_08e_free_will after coding_02_fizzbuzz_gen; research_08e_free_will after research_08e_free_will; coding_06_tax_rates_lan after research_08e_free_will; repoqa_05_retry_policy after coding_06_tax_rates_lan; refactor_02_rename_price after repoqa_05_retry_policy; repoqa_05_retry_policy after refactor_02_rename_price; comms_12_timezone_slot after repoqa_05_retry_policy; coding_02_fizzbuzz_gen after comms_12_timezone_slot; comms_07_confirm_send after coding_02_fizzbuzz_gen; coding_01_my_sqrt_review after comms_07_confirm_send; coding_06_tax_rates_lan after coding_01_my_sqrt_review; c_ringbuf_03_ringbuffer after coding_06_tax_rates_lan; coding_06_tax_rates_lan after c_ringbuf_03_ringbuffer; refactor_02_rename_price after coding_06_tax_rates_lan; bugfix_09_booking_slots after refactor_02_rename_price; longctx_06_tidal_report after bugfix_09_booking_slots; coding_03_compress_gen after longctx_06_tidal_report; logtriage_04_pool_exhaustion after coding_03_compress_gen; bugfix_09_booking_slots after logtriage_04_pool_exhaustion; repoqa_05_retry_policy after bugfix_09_booking_slots; refactor_02_rename_price after repoqa_05_retry_policy; coding_10_wcx_constrained after refactor_02_rename_price; coding_02_fizzbuzz_gen after coding_10_wcx_constrained; coding_04_dir_stats_gen after coding_02_fizzbuzz_gen; logtriage_04_pool_exhaustion after coding_04_dir_stats_gen; coding_06_tax_rates_lan after logtriage_04_pool_exhaustion; coding_02_fizzbuzz_gen after coding_06_tax_rates_lan; refactor_02_rename_price after coding_02_fizzbuzz_gen; coding_05_scientific_lan after refactor_02_rename_price; coding_04_dir_stats_gen after coding_05_scientific_lan; coding_04_dir_stats_gen after coding_04_dir_stats_gen; logtriage_04_pool_exhaustion after coding_04_dir_stats_gen; coding_05_scientific_lan after logtriage_04_pool_exhaustion; comms_12_timezone_slot after coding_05_scientific_lan; coding_03_compress_gen after comms_12_timezone_slot; coding_01_my_sqrt_review after coding_03_compress_gen; coding_10_wcx_constrained after coding_01_my_sqrt_review; coding_03_compress_gen after coding_10_wcx_constrained; longctx_11_harbor_report after coding_03_compress_gen; coding_05_scientific_lan after longctx_11_harbor_report; coding_06_tax_rates_lan after coding_05_scientific_lan; comms_12_timezone_slot after coding_06_tax_rates_lan; longctx_11_harbor_report after comms_12_timezone_slot; bugfix_01_moving_average after longctx_11_harbor_report; coding_02_fizzbuzz_gen after bugfix_01_moving_average; bugfix_09_booking_slots after coding_02_fizzbuzz_gen; coding_05_scientific_lan after bugfix_09_booking_slots; coding_01_my_sqrt_review after coding_05_scientific_lan; comms_12_timezone_slot after coding_01_my_sqrt_review; comms_07_confirm_send after comms_12_timezone_slot; research_08e_free_will after comms_07_confirm_send; repoqa_05_retry_policy after research_08e_free_will; bugfix_01_moving_average after repoqa_05_retry_policy; comms_12_timezone_slot after bugfix_01_moving_average; comms_07_confirm_send after comms_12_timezone_slot; bugfix_01_moving_average after comms_07_confirm_send; longctx_06_tidal_report after bugfix_01_moving_average; c_ringbuf_03_ringbuffer after longctx_06_tidal_report; coding_03_compress_gen after c_ringbuf_03_ringbuffer; comms_07_confirm_send after coding_03_compress_gen; logtriage_04_pool_exhaustion after comms_07_confirm_send; logtriage_04_pool_exhaustion after logtriage_04_pool_exhaustion; longctx_06_tidal_report after logtriage_04_pool_exhaustion; coding_01_my_sqrt_review after longctx_06_tidal_report; coding_04_dir_stats_gen after coding_01_my_sqrt_review; bugfix_09_booking_slots after coding_04_dir_stats_gen; repoqa_05_retry_policy after bugfix_09_booking_slots; coding_01_my_sqrt_review after repoqa_05_retry_policy.
