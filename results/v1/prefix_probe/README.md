# P12: what does the server's prompt cache reuse? (2026-10-25, 08:50-09:06)

Probe `scripts/p12_prefix_probe.py`: the first call of two recorded trials (`repoqa_05`, default agent; `comms_12`, custom `comms` agent) is sent
cold (nonce), then an edited copy; time to the first token of the edited call, mean of 2 repeats (`prefix_probe.md/json`, `../prefix_probe.log`).
A cold call takes 28-35 s (6.7-7.4k tokens), a fully cached one 0.4 s.

| Edit of the second call | repoqa_05 (default agent) | comms_12 (custom agent) |
|---|---:|---:|
| identical | 0.45 s | 0.37 s |
| other `seed` | 0.43 s | 0.38 s |
| text appended to the last user message | 2.6 s | 2.3 s |
| text appended to the end of the system message | 2.7 s | 2.3 s |
| **working directory in the system message changed** | **2.7 s (reused)** | **27.0 s (cold)** |
| first tool description edited | 30.6 s (cold) | 27.8 s (cold) |
| last tool description edited | 30.2 s (cold) | 27.6 s (cold) |

* The sampling seed does not matter; appending to the end of the prompt costs only the new tokens.
* **Any change inside the tool block loses the whole cache**, even to the last tool.
* **A changed working directory is free for the default agent, but costs a full cold prefill with a custom agent.** The environment block with the per-trial path sits
  at character 8 700 of 9 597 of the default system prompt, but at character 577 of 1 474 (`comms`) and 1 160 of 2 057 (`research`) in the custom agents, whose short
  prompts replace the built-in one. So every trial that uses a custom agent starts cold (~25-30 s of prefill), which explains why the cold first calls in the earlier
  runs were all `comms_07` / `research_08e` trials (their agents), and why `comms_12`'s warm replay missed the cache (see `../m5_promptreplay/README.md`).
  The mechanism (the model has 31 recurrent layers, so the server may keep state only at checkpoints) is a hypothesis, not tested.
* Consequence: a stable-prefix layout for custom agents (nothing trial-specific early) would save ~25-30 s per trial for `comms`, `research` and the verification
  agent; OpenCode decides where the environment block goes, so the lever is the length of the agent prompt, not tested here. It does not change success.
