# Results: m6_research

Run seed 4, n = 2 per cell, configurations: sw_64k (baseline: **sw_64k**). 40 measured trials, 0 harness failures excluded. Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_64k | 40 | 68% [52%, 80%] | 0 | 0 | 12032.8 [9371.4, 14694.2] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| research_m6_01_sleep_memory | sw_64k | 2* | 2 | 100% [34%, 100%] | 750.0 [581.4, 918.6] |
| research_m6_02_intermittent_fasting | sw_64k | 2* | 1 | 50% [9%, 91%] | 1323.6 [1253.2, 1394.0] |
| research_m6_03_growth_mindset | sw_64k | 2* | 2 | 100% [34%, 100%] | 470.9 [459.9, 481.8] |
| research_m6_04_minimum_wage | sw_64k | 2* | 1 | 50% [9%, 91%] | 297.2 [60.1, 534.2] |
| research_m6_05_microplastics | sw_64k | 2* | 2 | 100% [34%, 100%] | 510.9 [477.6, 544.2] |
| research_m6_06_replication_psychology | sw_64k | 2* | 1 | 50% [9%, 91%] | 436.4 [43.7, 829.1] |
| research_m6_07_mrna_vaccines | sw_64k | 2* | 1 | 50% [9%, 91%] | 734.2 [709.8, 758.6] |
| research_m6_08_remote_work | sw_64k | 2* | 1 | 50% [9%, 91%] | 623.6 [394.1, 853.1] |
| research_m6_09_transformer_scaling | sw_64k | 2* | 1 | 50% [9%, 91%] | 286.5 [273.9, 299.2] |
| research_m6_10_crispr_offtarget | sw_64k | 2* | 1 | 50% [9%, 91%] | 937.5 [519.1, 1355.8] |
| research_m6_11_urban_heat | sw_64k | 2* | 2 | 100% [34%, 100%] | 870.4 [482.4, 1258.4] |
| research_m6_12_antibiotic_resistance | sw_64k | 2* | 1 | 50% [9%, 91%] | 594.2 [576.4, 612.0] |
| research_m6_13_dunbar_number | sw_64k | 2* | 2 | 100% [34%, 100%] | 879.9 [844.8, 915.0] |
| research_m6_14_placebo_surgery | sw_64k | 2* | 0 | 0% [0%, 66%] | 405.1 [310.3, 500.0] |
| research_m6_15_nudge_effect | sw_64k | 2* | 2 | 100% [34%, 100%] | 354.6 [285.8, 423.4] |
| research_m6_16_exoplanet_habitability | sw_64k | 2* | 0 | 0% [0%, 66%] | 576.1 [531.4, 620.9] |
| research_m6_17_ancient_dna | sw_64k | 2* | 2 | 100% [34%, 100%] | 650.4 [517.0, 783.8] |
| research_m6_18_battery_solid_state | sw_64k | 2* | 2 | 100% [34%, 100%] | 627.8 [559.8, 695.9] |
| research_m6_19_vitamin_d | sw_64k | 2* | 1 | 50% [9%, 91%] | 210.6 [31.3, 389.8] |
| research_m6_20_social_media_teens | sw_64k | 2* | 2 | 100% [34%, 100%] | 492.9 [459.4, 526.4] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_64k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| research | 40 | 24066 | 58.9 | 31.1 | 5.8 | 4.2 | 0.0 |
| all tasks | 40 | 24066 | 58.9 | 31.1 | 5.8 | 4.2 | 0.0 |

NVML GPU activity (any kernel per sample window): 90% of wall-clock; first LLM call of a session = 5% of prefill time; 2 trial(s) flagged by the decomposition checks.

## Comparison with the baseline

Only one configuration: nothing to compare.

