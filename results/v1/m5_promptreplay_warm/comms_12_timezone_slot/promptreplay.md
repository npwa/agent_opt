# Prompt replay: results/v1/m5_specdecode/capture/proxy/requests/sw_32k__comms_12_timezone_slot__00

2 recorded calls, 2 interleaved repeats per variant, flush = `nonce`, max_tokens = 1; model qwen3.6-32k. Times are time-to-first-token (seconds).

| Variant | Cache | first-call prompt tokens | first call (s) | whole sequence (s) | vs default (sequence) |
|---|---|---:|---:|---:|---|
| default | cold | 6691 | 27.48 | 33.02 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | cold | 4404 | 18.31 | 23.81 | x0.72 [0.71, 0.73] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | cold | 4722 | 19.90 | 25.37 | x0.77 [0.76, 0.78] (directional, n < 5) |
| default | warm | 6649 | 26.69 | 32.20 |  |
| drop_tools=task+todowrite+skill+webfetch+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource | warm | 4392 | 18.04 | 23.54 | x0.73 [0.73, 0.73] (directional, n < 5) |
| drop_tools=task+todowrite+skill+webfetch | warm | 4692 | 19.64 | 25.18 | x0.78 [0.78, 0.79] (directional, n < 5) |

Cold = cache flushed before the sequence; warm = the sequence replayed right after another one (working directory changed). In fast mode follow-up calls re-process the previous reply (see the module docstring).
