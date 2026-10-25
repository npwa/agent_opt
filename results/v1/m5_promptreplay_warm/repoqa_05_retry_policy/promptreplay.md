# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__repoqa_05_retry_policy__00

3 recorded calls, 2 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 7395 | 30.28 | 36.28 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 5424 | 22.65 | 28.94 | x0.80 [0.78, 0.82] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 5426 | 23.38 | 29.67 | x0.82 [0.78, 0.86] (directional, n < 5) |
| default | warm | 7353 | 2.43 | 8.21 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 5412 | 2.54 | 9.47 | x1.15 [1.06, 1.25] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 5396 | 2.47 | 8.74 | x1.06 [1.03, 1.10] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
