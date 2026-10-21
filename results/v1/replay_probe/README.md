# Replay probe (plan item P7)

Two real trials (`coding_02_fizzbuzz_gen`, `repoqa_05_retry_policy`, one each, isolated OpenCode, `qwen3.6-32k`)
run through the proxy with `AGENT_OPT_CAPTURE=all`, which wrote the raw request body of every call to
`proxy/requests/<trial>/a1_NNN.json` (5 calls each). `replay_repoqa/` replays the `repoqa_05` sequence with
`python -m harness.promptreplay run` (default flush = a nonce at the start of the prompt, 3 interleaved repeats,
`max_tokens` = 1): the recorded prompt against the same prompt with four unused tool schemas dropped.
`replay_repoqa_dummy_flush/` is the first attempt, kept on purpose: its cold state was made by sending an
unrelated long prompt, which did **not** reliably evict the cache (two of six "cold" runs took 2.5 s, the warm
time), so the default was changed.

| Variant (7.4k-token prompt) | Cold first call | Warm first call |
|---|---:|---:|
| default | 31.25 s | 2.50 s |
| drop `task`, `todowrite`, `skill`, `webfetch` (-1 965 tokens) | 22.94 s | 2.48 s |

Dropping the four tools saves 8.3 s (27 %) on a cold first call, which is what 1 965 tokens at 4.2 ms/token
predicts, and nothing on a warm one. The spread over the repeats was about 0.05 s. The whole replayed sequence
(five calls) was 41.5 s vs 33.1 s cold and 12.3 s vs 12.9 s warm; the 4 % difference warm is not explained.
