# Prompt budget: baseline

140 trials, 714 LLM calls (proxy records).

## Prefill and cache

- Cold prefill rate: **253 tok/s** (fitted on follow-up calls). First call = 10142 prompt tokens; ttft median 40.6 s (min 37.0, max 44.2); 140 cold, 0 warm.
- Follow-up calls: ttft = 1.12 s + new tokens / rate (median abs. error 0.23 s); 97 % of the prompt is served from cache (median).
- First calls are 62 % of all prefill time (5677 s of 9150 s). That is 33 % of the summed trial time.
- Cross-trial reuse: 100 % of first calls are fully cold. Speed-up of a first call over a full cold prefill (median; 1.0 = no reuse): 1.0 after a different task, 1.0 after the same task (n = 10).

## Prompt components (one captured first call)

10142 prompt tokens, 39892 characters (about 3.9 chars/token); 10 tools; 12 skills listed. Token counts are estimates (characters apportioned to the real prompt size).

| Component | chars | tokens (est.) | share |
|---|---:|---:|---:|
| system prompt (base) | 8692 | 2210 | 22 % |
| environment block | 251 | 64 | 1 % |
| skills block | 10372 | 2637 | 26 % |
| tool schemas | 20491 | 5210 | 51 % |
| user prompt | 86 | 22 | 0 % |

| Tool | chars | tokens (est.) |
|---|---:|---:|
| bash | 5244 | 1333 |
| edit | 1927 | 490 |
| glob | 1083 | 275 |
| grep | 1152 | 293 |
| read | 1703 | 433 |
| skill | 645 | 164 |
| task | 3825 | 972 |
| todowrite | 2655 | 675 |
| webfetch | 1262 | 321 |
| write | 995 | 253 |
