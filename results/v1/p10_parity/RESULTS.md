# Results: p10_parity

Run seed 1, n = 12-30 per cell (varies by task), configurations: sw_32k (baseline: **sw_32k**). 96 measured trials, 0 harness failures excluded, 96 uninstrumented parity trials (see parity). Charts: `report.html`.

Platform: NVIDIA GeForce RTX 3080 (driver 580.126.09, CUDA 13.0), Ollama 0.32.5, OpenCode 1.18.11, llama.cpp b2f036c0f, Python 3.12.3.

## Configurations

| Configuration | trials | pass rate (95% Wilson) | timeouts | harness failures | suite time, sum of task medians (s) |
|---|---:|---:|---:|---:|---:|
| sw_32k | 96 | 58% [48%, 68%] | 0 | 0 | 166.8 [113.9, 205.4] |

## Per task

| Task | Configuration | n | pass | rate (95% Wilson) | median time s (95% bootstrap CI) |
|---|---|---:|---:|---|---|
| coding_01_my_sqrt_review | sw_32k | 12 | 7 | 58% [32%, 81%] | 36.5 [30.0, 42.0] |
| coding_05_scientific_lan | sw_32k | 30 | 29 | 97% [83%, 99%] | 11.5 [10.2, 12.5] |
| coding_06_tax_rates_lan | sw_32k | 30 | 8 | 27% [14%, 44%] | 14.1 [13.3, 15.5] |
| refactor_02_rename_price | sw_32k | 12 | 8 | 67% [39%, 86%] | 73.8 [33.0, 99.0] |
| repoqa_05_retry_policy | sw_32k | 12 | 4 | 33% [14%, 61%] | 30.8 [27.2, 36.4] |

\* directional: fewer than 5 trials

## Where the time goes

### sw_32k

| Category | n | wall (s) | prefill % | decode % | tool % | harness % | unattributed % |
|---|---:|---:|---:|---:|---:|---:|---:|
| code_review | 12 | 429 | 14.8 | 79.9 | 0.0 | 5.2 | 0.0 |
| reasoning | 60 | 838 | 24.7 | 62.3 | 0.0 | 13.0 | 0.0 |
| refactor | 12 | 953 | 37.0 | 53.6 | 0.0 | 9.4 | 0.0 |
| repo_qa | 12 | 392 | 32.5 | 61.6 | 0.0 | 5.9 | 0.0 |
| all tasks | 96 | 2612 | 28.7 | 61.9 | 0.0 | 9.3 | 0.0 |

NVML GPU activity (any kernel per sample window): 87% of wall-clock; first LLM call of a session = 74% of prefill time; 2 trial(s) flagged by the decomposition checks.

## Comparison with the baseline

Only one configuration: nothing to compare.

## Instrumentation parity

Verdict: **inconclusive** (96 uninstrumented trials). See `parity.md`.

