# Prompt replay: results/v1/replay_probe/proxy/requests/baseline_32k__repoqa_05_retry_policy__00

5 recorded calls, 3 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 7383 | 31.25 | 41.47 |  |
| drop_tools=task+todowrite+skill+webfetch | cold | 5418 | 22.94 | 33.11 | x0.80 [0.80, 0.80] (directional, n < 5) |
| default | warm | 7346 | 2.50 | 12.31 |  |
| drop_tools=task+todowrite+skill+webfetch | warm | 5389 | 2.48 | 12.85 | x1.04 [1.04, 1.05] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
