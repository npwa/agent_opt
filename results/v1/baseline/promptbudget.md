# Prompt budget: baseline

140 trials, 714 LLM calls (proxy records).

## Prefill and cache

- Cold prefill rate (first calls): **250 tok/s** (min 237, max 253); first call = 10142 prompt tokens = 40.6 s.
- Follow-up calls: ttft = 1.10 s + new tokens / rate (median abs. error 0.25 s); 97 % of the prompt is served from cache (median).
- First calls are 62 % of all prefill time (5677 s of 9150 s). That is 33 % of the summed trial time.
- Cross-trial reuse: 99 % of first calls run within 5 % of the cold rate; rate relative to cold is 1.00 after a different task and 1.00 after the same task (n = 10). A value near 1.00 means no prefix is reused between trials.

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
