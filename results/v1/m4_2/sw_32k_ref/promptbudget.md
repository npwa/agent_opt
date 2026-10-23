# Prompt budget: sw_32k_ref

80 trials, 430 LLM calls (proxy records).

## Prefill and cache

- Cold prefill rate: **250 tok/s** (fitted on follow-up calls). First call = 7342 prompt tokens; ttft median 30.4 s (min 26.7, max 34.1); 80 cold, 0 warm.
- Follow-up calls: ttft = 1.12 s + new tokens / rate (median abs. error 0.27 s); 97 % of the prompt is served from cache (median).
- First calls are 56 % of all prefill time (2412 s of 4318 s). That is 23 % of the summed trial time.
- Cross-trial reuse: 100 % of first calls are fully cold. Speed-up of a first call over a full cold prefill (median; 1.0 = no reuse): 1.0 after a different task, 1.0 after the same task (n = 3).
