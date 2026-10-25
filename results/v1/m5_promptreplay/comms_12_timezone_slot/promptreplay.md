# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__comms_12_timezone_slot__00

2 recorded calls, 3 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 6691 | 27.68 | 33.17 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 4404 | 18.39 | 23.88 | x0.72 [0.71, 0.72] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 4718 | 19.99 | 25.47 | x0.77 [0.76, 0.77] (directional, n < 5) |
| strip_skills | cold | 6526 | 26.83 | 32.35 | x0.98 [0.97, 0.98] (directional, n < 5) |
| default | warm | 6653 | 26.88 | 32.39 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 4424 | 18.38 | 23.91 | x0.74 [0.73, 0.74] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 4708 | 19.84 | 25.33 | x0.78 [0.77, 0.79] (directional, n < 5) |
| strip_skills | warm | 6492 | 26.65 | 32.17 | x0.99 [0.98, 0.99] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
