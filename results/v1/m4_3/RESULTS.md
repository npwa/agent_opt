# Share of wall-clock the GPU is actually working

20 Nsight Systems captures (one trial each), 10 task categories. 'GPU active' = time with at least one kernel or CUDA graph running on the GPU, from the Nsight trace; NVML is the coarse 'any kernel in the sample window' signal used by the earlier reports.

**All captures together: the GPU is active 11.7 % of the wall-clock** (284 s of 2420 s); prefill is 55 % and decode 42 % of the wall-clock.

## Per task category (mean of its captures)

| Category | captures | wall s | prefill s | decode s | GPU active s | GPU active % of wall | % of prefill | % of decode | NVML busy % of LLM time |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| bugfix | 2 | 107 | 55 | 50 | 12.6 | 11.4 | 8.3 | 15.9 | 50 |
| code_gen | 2 | 64 | 38 | 24 | 6.9 | 10.8 | 8.5 | 15.2 | 52 |
| code_review | 2 | 68 | 34 | 32 | 7.8 | 11.4 | 8.6 | 15.2 | 53 |
| comms | 2 | 180 | 41 | 130 | 22.7 | 12.6 | 7.9 | 15.1 | 37 |
| log_triage | 2 | 135 | 68 | 64 | 16.3 | 12.1 | 8.7 | 16.1 | 54 |
| long_context | 2 | 129 | 86 | 40 | 14.3 | 10.9 | 9.3 | 15.1 | 64 |
| reasoning | 2 | 42 | 31 | 9 | 3.8 | 9.1 | 8.7 | 12.1 | 80 |
| refactor | 2 | 135 | 63 | 70 | 17.7 | 13.0 | 9.3 | 16.9 | 46 |
| repo_qa | 2 | 55 | 40 | 12 | 4.9 | 8.9 | 8.9 | 10.5 | 65 |
| research | 2 | 297 | 207 | 83 | 35.0 | 11.9 | 9.8 | 17.4 | 71 |

## Per capture

| Capture | status | wall s | steps | GPU active % of wall | % of prefill | % of decode | NVML % | decode gap p50 / p99 ms |
|---|---|---:|---:|---:|---:|---:|---:|---|
| bugfix__bugfix_01_moving_average__rep1 | pass | 69 | 5 | 10.5 | 8.2 | 15.6 | 55 | 0.71 / 2.8 |
| bugfix__bugfix_09_booking_slots__rep2 | fail | 145 | 12 | 12.4 | 8.4 | 16.2 | 45 | 0.71 / 3.3 |
| code_gen__coding_02_fizzbuzz_gen__rep1 | pass | 59 | 4 | 10.1 | 8.2 | 14.5 | 53 | 0.67 / 2.6 |
| code_gen__coding_03_compress_gen__rep2 | pass | 69 | 4 | 11.5 | 8.8 | 16.0 | 52 | 0.70 / 2.6 |
| code_review__coding_01_my_sqrt_review__rep1 | pass | 68 | 2 | 12.1 | 8.7 | 16.1 | 56 | 0.71 / 3.4 |
| code_review__coding_01_my_sqrt_review__rep2 | fail | 68 | 3 | 10.8 | 8.4 | 14.2 | 50 | 0.73 / 3.5 |
| comms__comms_07_confirm_send__rep1 | pass | 164 | 10 | 12.0 | 7.5 | 15.5 | 39 | 0.70 / 3.1 |
| comms__comms_12_timezone_slot__rep2 | fail | 195 | 3 | 13.2 | 8.3 | 14.7 | 35 | 0.78 / 3.8 |
| log_triage__logtriage_04_pool_exhaustion__rep1 | pass | 137 | 5 | 12.2 | 9.0 | 16.1 | 58 | 0.72 / 3.4 |
| log_triage__logtriage_04_pool_exhaustion__rep2 | pass | 133 | 8 | 12.0 | 8.5 | 16.1 | 50 | 0.72 / 3.4 |
| long_context__longctx_06_tidal_report__rep1 | pass | 98 | 3 | 10.0 | 9.2 | 13.9 | 68 | 0.72 / 2.9 |
| long_context__longctx_11_harbor_report__rep2 | pass | 159 | 4 | 11.9 | 9.4 | 16.2 | 61 | 0.77 / 3.7 |
| reasoning__coding_05_scientific_lan__rep1 | pass | 39 | 1 | 8.8 | 8.7 | 11.6 | 83 | 0.69 / 2.5 |
| reasoning__coding_06_tax_rates_lan__rep2 | fail | 44 | 1 | 9.4 | 8.7 | 12.7 | 77 | 0.73 / 3.6 |
| refactor__refactor_02_rename_price__rep1 | pass | 164 | 11 | 13.5 | 9.8 | 17.0 | 42 | 0.68 / 2.7 |
| refactor__refactor_02_rename_price__rep2 | pass | 107 | 6 | 12.5 | 8.9 | 16.8 | 50 | 0.68 / 12.0 |
| repo_qa__repoqa_05_retry_policy__rep1 | pass | 55 | 3 | 8.9 | 9.1 | 9.8 | 64 | 0.66 / 3.5 |
| repo_qa__repoqa_05_retry_policy__rep2 | pass | 55 | 3 | 9.0 | 8.7 | 11.2 | 67 | 0.67 / 2.7 |
| research__research_08e_free_will__rep1 | fail | 426 | 9 | 11.7 | 9.9 | 17.6 | 68 | 0.70 / 2.7 |
| research__research_08e_free_will__rep2 | fail | 168 | 3 | 12.1 | 9.7 | 17.1 | 74 | 0.71 / 3.3 |
