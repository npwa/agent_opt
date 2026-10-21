# Prompt budget: baseline_isolated

140 trials, 756 LLM calls (proxy records).

## Prefill and cache

- Cold prefill rate: **257 tok/s** (fitted on follow-up calls). First call = 7326 prompt tokens; ttft median 2.7 s (min 2.4, max 52.1); 10 cold, 130 warm.
- Follow-up calls: ttft = 1.20 s + new tokens / rate (median abs. error 0.27 s); 97 % of the prompt is served from cache (median).
- First calls are 15 % of all prefill time (682 s of 4404 s). That is 5 % of the summed trial time.
- Cross-trial reuse: 7 % of first calls are fully cold. Speed-up of a first call over a full cold prefill (median; 1.0 = no reuse): 10.5 after a different task, 11.4 after the same task (n = 10).

## Prompt components (one captured first call)

7326 prompt tokens, 30223 characters (about 4.1 chars/token); 10 tools; 1 skills listed. Token counts are estimates (characters apportioned to the real prompt size).

| Component | chars | tokens (est.) | share |
|---|---:|---:|---:|
| system prompt (base) | 8692 | 2107 | 29 % |
| environment block | 253 | 61 | 1 % |
| skills block | 701 | 170 | 2 % |
| tool schemas | 20491 | 4967 | 68 % |
| user prompt | 86 | 21 | 0 % |

| Tool | chars | tokens (est.) |
|---|---:|---:|
| bash | 5244 | 1271 |
| edit | 1927 | 467 |
| glob | 1083 | 263 |
| grep | 1152 | 279 |
| read | 1703 | 413 |
| skill | 645 | 156 |
| task | 3825 | 927 |
| todowrite | 2655 | 644 |
| webfetch | 1262 | 306 |
| write | 995 | 241 |
