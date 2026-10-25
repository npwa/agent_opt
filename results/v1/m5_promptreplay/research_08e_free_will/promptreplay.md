# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__research_08e_free_will__00

4 recorded calls, 3 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 7684 | 31.22 | 183.61 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 5397 | 22.68 | 173.76 | x0.95 [0.94, 0.95] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 5711 | 23.95 | 174.67 | x0.95 [0.95, 0.95] (directional, n < 5) |
| strip_skills | cold | 7519 | 31.15 | 183.63 | x1.00 [1.00, 1.00] (directional, n < 5) |
| default | warm | 7645 | 30.89 | 183.30 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 5416 | 22.50 | 173.88 | x0.95 [0.95, 0.95] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 5700 | 23.57 | 174.97 | x0.95 [0.95, 0.96] (directional, n < 5) |
| strip_skills | warm | 7484 | 30.81 | 183.46 | x1.00 [1.00, 1.00] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
