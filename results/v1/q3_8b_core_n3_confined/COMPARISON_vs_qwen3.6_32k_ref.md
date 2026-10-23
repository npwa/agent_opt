# Comparison: sw_32k_ref (A) vs q3_8b_core_n3_confined (B)

14 common tasks. Differences are B - A. Intervals: Wilson (rates), Newcombe (rate differences), percentile bootstrap (times); the number of trials per task is in the pass counts (A pass, B pass).

## Success

- Overall: A 41/62 = 66 % [54 %, 77 %]; B 10/42 = 24 % [13 %, 39 %]; difference -42.3 pp [-57.1, -23.1].

| Task | A pass | B pass | difference (95 % CI), pp |
|---|---:|---:|---|
| bugfix_09_booking_slots | 2/5 | 0/3 | -40 [-77, +23] |
| c_ringbuf_03_ringbuffer | 2/3 | 0/3 | -67 [-94, +6] |
| coding_01_my_sqrt_review | 4/5 | 0/3 | -80 [-96, -10] |
| coding_02_fizzbuzz_gen | 5/5 | 2/3 | -33 [-79, +18] |
| coding_03_compress_gen | 5/5 | 0/3 | -100 [-100, -29] |
| coding_05_scientific_lan | 5/5 | 3/3 | +0 [-56, +43] |
| coding_06_tax_rates_lan | 1/5 | 3/3 | +80 [+10, +96] |
| coding_10_wcx_constrained | 1/3 | 0/3 | -33 [-79, +29] |
| comms_12_timezone_slot | 4/5 | 0/3 | -80 [-96, -10] |
| logtriage_04_pool_exhaustion | 4/5 | 0/3 | -80 [-96, -10] |
| longctx_11_harbor_report | 2/3 | 0/3 | -67 [-94, +6] |
| refactor_02_rename_price | 3/5 | 2/3 | +7 [-47, +53] |
| repoqa_05_retry_policy | 3/5 | 0/3 | -60 [-88, +7] |
| research_08e_free_will | 0/3 | 0/3 | +0 [-56, +56] |

## Time per trial

| Task | A median s | B median s | ratio B/A (95 % bootstrap CI) | Mann-Whitney p |
|---|---:|---:|---|---:|
| bugfix_09_booking_slots | 149.2 | 321.8 | 2.16 [0.31, 6.52] | 0.571 |
| c_ringbuf_03_ringbuffer | 528.5 | 177.9 | 0.34 [0.06, 1.25] | 0.4 |
| coding_01_my_sqrt_review | 73.6 | 19.4 | 0.26 [0.22, 0.34] | 0.0358 |
| coding_02_fizzbuzz_gen | 65.9 | 24.3 | 0.37 [0.15, 0.77] | 0.0357 |
| coding_03_compress_gen | 59.2 | 47.2 | 0.80 [0.38, 1.08] | 0.25 |
| coding_05_scientific_lan | 40.8 | 24.1 | 0.59 [0.38, 0.62] | 0.0358 |
| coding_06_tax_rates_lan | 41.0 | 7.9 | 0.19 [0.17, 0.21] | 0.0357 |
| coding_10_wcx_constrained | 280.9 | 165.9 | 0.59 [0.25, 3.18] | 0.7 |
| comms_12_timezone_slot | 224.6 | 193.8 | 0.86 [0.22, 1.10] | 0.143 |
| logtriage_04_pool_exhaustion | 127.9 | 45.0 | 0.35 [0.24, 0.52] | 0.0357 |
| longctx_11_harbor_report | 199.3 | 33.8 | 0.17 [0.10, 0.30] | 0.1 |
| refactor_02_rename_price | 110.3 | 25.0 | 0.23 [0.16, 0.46] | 0.0357 |
| repoqa_05_retry_policy | 53.9 | 20.4 | 0.38 [0.24, 0.58] | 0.0357 |
| research_08e_free_will | 369.6 | 40.1 | 0.11 [0.09, 0.31] | 0.1 |

- Pooled, normalised by each task's A median: median ratio B/A = 0.34 [0.25, 0.48] (n = 62 vs 42, Mann-Whitney p = 8.08e-12).
- Mean time per trial: A 147 s, B 83 s (-44 %). Suite time (sum of task medians): A 2673 s, B 1147 s.

## Where the time goes (all tasks, share of wall-clock)

| Bucket | A | B |
|---|---:|---:|
| prefill | 42.1 % | 4.6 % |
| decode | 52.5 % | 85.0 % |
| tool | 2.9 % | 7.8 % |
| harness | 3.5 % | 2.6 % |
| unattributed | -1.0 % | -0.0 % |
| wall-clock per trial | 133 s | 83 s |

## Prompt and cache

| | A | B |
|---|---:|---:|
| first-call prompt tokens (median) | 7342 | 6470 |
| cold prefill rate (tok/s) | 250 | 2714 |
| first-call ttft, median (s) | 30.4 | 1.8 |
| first calls cold / warm | 80 / 0 | 10 / 32 |
| first calls, share of prefill time | 56 % | 48 % |

### Order dependence in B (first call of a trial vs the previous trial, 41 pairs)

| previous trial's first prompt | first call cold | first call warm |
|---|---:|---:|
| same size (within 30 tokens) | 0 | 12 |
| different size | 10 | 19 |

Cold first calls (task, previous task): research_08e_free_will after bugfix_09_booking_slots; c_ringbuf_03_ringbuffer after logtriage_04_pool_exhaustion; comms_12_timezone_slot after research_08e_free_will; research_08e_free_will after repoqa_05_retry_policy; coding_03_compress_gen after longctx_11_harbor_report; comms_12_timezone_slot after coding_03_compress_gen; coding_05_scientific_lan after bugfix_09_booking_slots; comms_12_timezone_slot after coding_03_compress_gen; logtriage_04_pool_exhaustion after longctx_11_harbor_report; coding_01_my_sqrt_review after longctx_11_harbor_report.
