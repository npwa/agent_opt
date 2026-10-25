# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__bugfix_09_booking_slots__00

13 recorded calls, 3 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 7419 | 30.42 | 60.36 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 5448 | 22.31 | 51.84 | x0.86 [0.82, 0.93] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 5446 | 22.33 | 51.96 | x0.86 [0.83, 0.96] (directional, n < 5) |
| strip_skills | cold | 7254 | 30.66 | 61.30 | x1.02 [0.95, 1.09] (directional, n < 5) |
| default | warm | 7381 | 30.23 | 59.44 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 5468 | 22.26 | 52.73 | x0.89 [0.85, 1.00] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 5436 | 22.24 | 52.13 | x0.88 [0.84, 0.94] (directional, n < 5) |
| strip_skills | warm | 7220 | 29.71 | 59.30 | x1.00 [0.96, 1.24] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
