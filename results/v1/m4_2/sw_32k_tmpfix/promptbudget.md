# Prompt budget: sw_32k_tmpfix

24 trials, 102 LLM calls (proxy records).

## Prefill and cache

- Cold prefill rate: **304 tok/s** (fitted on follow-up calls). First call = 7325 prompt tokens; ttft median 2.5 s (min 2.4, max 30.4); 2 cold, 22 warm.
- Follow-up calls: ttft = 1.37 s + new tokens / rate (median abs. error 0.19 s); 97 % of the prompt is served from cache (median).
- First calls are 38 % of all prefill time (112 s of 298 s). That is 11 % of the summed trial time.
- Cross-trial reuse: 8 % of first calls are fully cold. Speed-up of a first call over a full cold prefill (median; 1.0 = no reuse): 9.8 after a different task, 9.7 after the same task (n = 3).
