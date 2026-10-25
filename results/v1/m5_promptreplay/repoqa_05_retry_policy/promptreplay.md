# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__repoqa_05_retry_policy__00

3 recorded calls, 3 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 7395 | 30.34 | 36.40 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 5424 | 22.22 | 28.19 | x0.77 [0.77, 1.47] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 5422 | 22.27 | 28.28 | x0.78 [0.77, 0.78] (directional, n < 5) |
| strip_skills | cold | 7230 | 29.69 | 35.71 | x0.98 [0.97, 0.99] (directional, n < 5) |
| default | warm | 7357 | 30.14 | 35.91 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 5444 | 22.34 | 28.77 | x0.80 [0.80, 0.81] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 5412 | 22.17 | 28.25 | x0.79 [0.79, 0.79] (directional, n < 5) |
| strip_skills | warm | 7196 | 28.82 | 34.65 | x0.96 [0.96, 0.97] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
