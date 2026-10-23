# Comparison: sw_32k_ref (A) vs sw_16k (B)

18 common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), percentile bootstrap (times); the number of trials per task is in the pass counts (A pass, B pass).

## Success

- Overall: A 58/80 = 72 % [62 %, 81 %]; B 104/159 = 65 % [58 %, 72 %]; difference -7.1 pp [-18.6, +5.6].

| Task | A pass | B pass | difference (95 % CI), pp |
|---|---:|---:|---|
| bugfix_01_moving_average | 5/5 | 10/10 | +0 [-28, +43] |
| bugfix_09_booking_slots | 2/5 | 4/10 | +0 [-44, +40] |
| c_ringbuf_03_ringbuffer | 2/3 | 6/10 | -7 [-46, +45] |
| coding_01_my_sqrt_review | 4/5 | 6/10 | -20 [-53, +28] |
| coding_02_fizzbuzz_gen | 5/5 | 10/10 | +0 [-28, +43] |
| coding_03_compress_gen | 5/5 | 8/10 | -20 [-51, +26] |
| coding_04_dir_stats_gen | 4/5 | 10/10 | +20 [-12, +62] |
| coding_05_scientific_lan | 5/5 | 10/10 | +0 [-28, +43] |
| coding_06_tax_rates_lan | 1/5 | 3/10 | +10 [-37, +44] |
| coding_10_wcx_constrained | 1/3 | 4/10 | +7 [-45, +46] |
| comms_07_confirm_send | 5/5 | 10/10 | +0 [-28, +43] |
| comms_12_timezone_slot | 4/5 | 5/10 | -30 [-61, +20] |
| logtriage_04_pool_exhaustion | 4/5 | 1/10 | -70 [-88, -18] |
| longctx_06_tidal_report | 3/3 | 0/3 | -100 [-100, -21] |
| longctx_11_harbor_report | 2/3 | 0/3 | -67 [-94, +6] |
| refactor_02_rename_price | 3/5 | 9/10 | +30 [-12, +68] |
| repoqa_05_retry_policy | 3/5 | 7/10 | +10 [-31, +52] |
| research_08e_free_will | 0/3 | 1/3 | +33 [-29, +79] |

## Time per trial

| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |
|---|---:|---:|---|---:|
| bugfix_01_moving_average | 62.0 | 61.0 | 0.98 [0.88, 1.13] | 0.768 |
| bugfix_09_booking_slots | 149.2 | 124.4 | 0.83 [0.71, 1.58] | 0.713 |
| c_ringbuf_03_ringbuffer | 528.5 | 279.4 | 0.53 [0.31, 2.05] | 0.469 |
| coding_01_my_sqrt_review | 73.6 | 63.9 | 0.87 [0.74, 1.11] | 0.244 |
| coding_02_fizzbuzz_gen | 65.9 | 58.3 | 0.89 [0.68, 1.18] | 0.31 |
| coding_03_compress_gen | 59.2 | 65.0 | 1.10 [0.85, 1.39] | 0.679 |
| coding_04_dir_stats_gen | 70.5 | 71.6 | 1.02 [0.73, 1.49] | 0.581 |
| coding_05_scientific_lan | 40.8 | 40.2 | 0.99 [0.88, 1.04] | 0.624 |
| coding_06_tax_rates_lan | 41.0 | 40.5 | 0.99 [0.89, 1.03] | 0.5 |
| coding_10_wcx_constrained | 280.9 | 264.6 | 0.94 [0.29, 4.96] | 1 |
| comms_07_confirm_send | 124.3 | 105.6 | 0.85 [0.82, 1.04] | 0.31 |
| comms_12_timezone_slot | 224.6 | 175.3 | 0.78 [0.19, 0.99] | 0.0753 |
| logtriage_04_pool_exhaustion | 127.9 | 137.8 | 1.08 [0.73, 1.28] | 0.679 |
| longctx_06_tidal_report | 91.5 | 117.9 | 1.29 [1.22, 1.49] | 0.1 |
| longctx_11_harbor_report | 199.3 | 103.5 | 0.52 [0.36, 1.28] | 0.2 |
| refactor_02_rename_price | 110.3 | 98.8 | 0.90 [0.57, 1.82] | 0.679 |
| repoqa_05_retry_policy | 53.9 | 55.6 | 1.03 [0.84, 1.14] | 0.951 |
| research_08e_free_will | 369.6 | 144.8 | 0.39 [0.23, 1.47] | 0.2 |

- Pooled, normalised by each task's A median: median ratio B/A = 0.94 [0.90, 0.98] (n = 80 vs 159, Mann-Whitney p = 0.0197).
- Mean time per trial: A 133 s, B 114 s (-14 %). Suite time (sum of task medians): A 2673 s, B 2008 s.

## Where the time goes (all tasks, share of wall-clock)

| Bucket | A | B |
|---|---:|---:|
| prefill | 42.1 % | 41.9 % |
| decode | 52.5 % | 55.6 % |
| tool | 2.9 % | 0.1 % |
| harness | 3.5 % | 2.5 % |
| unattributed | -1.0 % | -0.0 % |
| wall-clock per trial | 133 s | 114 s |

## Prompt and cache

| | A | B |
|---|---:|---:|
| first-call prompt tokens (median) | 7342 | 7340 |
| cold prefill rate (tok/s) | 250 | 268 |
| first-call ttft, median (s) | 30.4 | 29.7 |
| first calls cold / warm | 80 / 0 | 159 / 0 |
| first calls, share of prefill time | 56 % | 64 % |

### Order dependence in B (first call of a trial vs the previous trial, 158 pairs)

| previous trial's first prompt | first call cold | first call warm |
|---|---:|---:|
| same size (within 30 tokens) | 66 | 0 |
| different size | 92 | 0 |

Cold first calls (task, previous task): coding_05_scientific_lan after comms_12_timezone_slot; comms_12_timezone_slot after coding_05_scientific_lan; refactor_02_rename_price after comms_12_timezone_slot; coding_06_tax_rates_lan after refactor_02_rename_price; bugfix_01_moving_average after coding_06_tax_rates_lan; longctx_11_harbor_report after bugfix_01_moving_average; comms_12_timezone_slot after longctx_11_harbor_report; coding_05_scientific_lan after comms_12_timezone_slot; bugfix_09_booking_slots after coding_05_scientific_lan; coding_10_wcx_constrained after bugfix_09_booking_slots; coding_02_fizzbuzz_gen after coding_10_wcx_constrained; coding_02_fizzbuzz_gen after coding_02_fizzbuzz_gen; bugfix_01_moving_average after coding_02_fizzbuzz_gen; longctx_06_tidal_report after bugfix_01_moving_average; research_08e_free_will after longctx_06_tidal_report; coding_04_dir_stats_gen after research_08e_free_will; longctx_06_tidal_report after coding_04_dir_stats_gen; comms_07_confirm_send after longctx_06_tidal_report; coding_03_compress_gen after comms_07_confirm_send; bugfix_09_booking_slots after coding_03_compress_gen; coding_04_dir_stats_gen after bugfix_09_booking_slots; bugfix_09_booking_slots after coding_04_dir_stats_gen; coding_02_fizzbuzz_gen after bugfix_09_booking_slots; coding_02_fizzbuzz_gen after coding_02_fizzbuzz_gen; repoqa_05_retry_policy after coding_02_fizzbuzz_gen; logtriage_04_pool_exhaustion after repoqa_05_retry_policy; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; refactor_02_rename_price after coding_02_fizzbuzz_gen; coding_06_tax_rates_lan after refactor_02_rename_price; repoqa_05_retry_policy after coding_06_tax_rates_lan; refactor_02_rename_price after repoqa_05_retry_policy; coding_05_scientific_lan after refactor_02_rename_price; coding_03_compress_gen after coding_05_scientific_lan; comms_12_timezone_slot after coding_03_compress_gen; repoqa_05_retry_policy after comms_12_timezone_slot; refactor_02_rename_price after repoqa_05_retry_policy; c_ringbuf_03_ringbuffer after refactor_02_rename_price; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; coding_06_tax_rates_lan after coding_05_scientific_lan; coding_01_my_sqrt_review after coding_06_tax_rates_lan; bugfix_01_moving_average after coding_01_my_sqrt_review; logtriage_04_pool_exhaustion after bugfix_01_moving_average; coding_10_wcx_constrained after logtriage_04_pool_exhaustion; coding_06_tax_rates_lan after coding_10_wcx_constrained; repoqa_05_retry_policy after coding_06_tax_rates_lan; coding_05_scientific_lan after repoqa_05_retry_policy; comms_07_confirm_send after coding_05_scientific_lan; bugfix_09_booking_slots after comms_07_confirm_send; coding_10_wcx_constrained after bugfix_09_booking_slots; coding_03_compress_gen after coding_10_wcx_constrained; coding_10_wcx_constrained after coding_03_compress_gen; coding_06_tax_rates_lan after coding_10_wcx_constrained; coding_01_my_sqrt_review after coding_06_tax_rates_lan; c_ringbuf_03_ringbuffer after coding_01_my_sqrt_review; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; coding_01_my_sqrt_review after bugfix_09_booking_slots; longctx_06_tidal_report after coding_01_my_sqrt_review; research_08e_free_will after longctx_06_tidal_report; logtriage_04_pool_exhaustion after research_08e_free_will; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; refactor_02_rename_price after coding_02_fizzbuzz_gen; coding_05_scientific_lan after refactor_02_rename_price; logtriage_04_pool_exhaustion after coding_05_scientific_lan; coding_10_wcx_constrained after logtriage_04_pool_exhaustion; comms_07_confirm_send after coding_10_wcx_constrained; c_ringbuf_03_ringbuffer after comms_07_confirm_send; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; coding_01_my_sqrt_review after coding_05_scientific_lan; refactor_02_rename_price after coding_01_my_sqrt_review; coding_04_dir_stats_gen after refactor_02_rename_price; bugfix_01_moving_average after coding_04_dir_stats_gen; coding_04_dir_stats_gen after bugfix_01_moving_average; refactor_02_rename_price after coding_04_dir_stats_gen; comms_07_confirm_send after refactor_02_rename_price; coding_04_dir_stats_gen after comms_07_confirm_send; c_ringbuf_03_ringbuffer after coding_04_dir_stats_gen; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; repoqa_05_retry_policy after bugfix_09_booking_slots; logtriage_04_pool_exhaustion after repoqa_05_retry_policy; bugfix_09_booking_slots after logtriage_04_pool_exhaustion; coding_02_fizzbuzz_gen after bugfix_09_booking_slots; logtriage_04_pool_exhaustion after coding_02_fizzbuzz_gen; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; c_ringbuf_03_ringbuffer after coding_02_fizzbuzz_gen; research_08e_free_will after c_ringbuf_03_ringbuffer; comms_12_timezone_slot after research_08e_free_will; coding_03_compress_gen after comms_12_timezone_slot; coding_01_my_sqrt_review after coding_03_compress_gen; coding_04_dir_stats_gen after coding_01_my_sqrt_review; bugfix_01_moving_average after coding_04_dir_stats_gen; comms_07_confirm_send after bugfix_01_moving_average; coding_03_compress_gen after comms_07_confirm_send; comms_07_confirm_send after coding_03_compress_gen; coding_04_dir_stats_gen after comms_07_confirm_send; coding_05_scientific_lan after coding_04_dir_stats_gen; coding_01_my_sqrt_review after coding_05_scientific_lan; coding_01_my_sqrt_review after coding_01_my_sqrt_review; repoqa_05_retry_policy after coding_01_my_sqrt_review; coding_06_tax_rates_lan after repoqa_05_retry_policy; comms_07_confirm_send after coding_06_tax_rates_lan; coding_03_compress_gen after comms_07_confirm_send; coding_04_dir_stats_gen after coding_03_compress_gen; coding_10_wcx_constrained after coding_04_dir_stats_gen; comms_07_confirm_send after coding_10_wcx_constrained; coding_02_fizzbuzz_gen after comms_07_confirm_send; coding_10_wcx_constrained after coding_02_fizzbuzz_gen; bugfix_09_booking_slots after coding_10_wcx_constrained; comms_12_timezone_slot after bugfix_09_booking_slots; coding_10_wcx_constrained after comms_12_timezone_slot; coding_06_tax_rates_lan after coding_10_wcx_constrained; c_ringbuf_03_ringbuffer after coding_06_tax_rates_lan; bugfix_09_booking_slots after c_ringbuf_03_ringbuffer; coding_06_tax_rates_lan after bugfix_09_booking_slots; coding_05_scientific_lan after coding_06_tax_rates_lan; comms_07_confirm_send after coding_05_scientific_lan; repoqa_05_retry_policy after comms_07_confirm_send; longctx_11_harbor_report after repoqa_05_retry_policy; coding_01_my_sqrt_review after longctx_11_harbor_report; refactor_02_rename_price after coding_01_my_sqrt_review; longctx_11_harbor_report after refactor_02_rename_price; c_ringbuf_03_ringbuffer after longctx_11_harbor_report; coding_06_tax_rates_lan after c_ringbuf_03_ringbuffer; logtriage_04_pool_exhaustion after coding_06_tax_rates_lan; coding_02_fizzbuzz_gen after logtriage_04_pool_exhaustion; c_ringbuf_03_ringbuffer after coding_02_fizzbuzz_gen; coding_05_scientific_lan after c_ringbuf_03_ringbuffer; repoqa_05_retry_policy after coding_05_scientific_lan; comms_12_timezone_slot after repoqa_05_retry_policy; coding_03_compress_gen after comms_12_timezone_slot; refactor_02_rename_price after coding_03_compress_gen; comms_07_confirm_send after refactor_02_rename_price; coding_03_compress_gen after comms_07_confirm_send; repoqa_05_retry_policy after coding_03_compress_gen; bugfix_01_moving_average after repoqa_05_retry_policy; bugfix_01_moving_average after bugfix_01_moving_average; bugfix_01_moving_average after bugfix_01_moving_average; repoqa_05_retry_policy after bugfix_01_moving_average; coding_06_tax_rates_lan after repoqa_05_retry_policy; c_ringbuf_03_ringbuffer after coding_06_tax_rates_lan; coding_03_compress_gen after c_ringbuf_03_ringbuffer; coding_04_dir_stats_gen after coding_03_compress_gen; comms_12_timezone_slot after coding_04_dir_stats_gen; bugfix_01_moving_average after comms_12_timezone_slot; comms_12_timezone_slot after bugfix_01_moving_average; coding_10_wcx_constrained after comms_12_timezone_slot; bugfix_01_moving_average after coding_10_wcx_constrained; logtriage_04_pool_exhaustion after bugfix_01_moving_average; c_ringbuf_03_ringbuffer after logtriage_04_pool_exhaustion; coding_03_compress_gen after c_ringbuf_03_ringbuffer; coding_10_wcx_constrained after coding_03_compress_gen; logtriage_04_pool_exhaustion after coding_10_wcx_constrained; comms_12_timezone_slot after logtriage_04_pool_exhaustion; logtriage_04_pool_exhaustion after comms_12_timezone_slot; coding_01_my_sqrt_review after logtriage_04_pool_exhaustion; coding_04_dir_stats_gen after coding_01_my_sqrt_review; bugfix_09_booking_slots after coding_04_dir_stats_gen; refactor_02_rename_price after bugfix_09_booking_slots; coding_01_my_sqrt_review after refactor_02_rename_price.
