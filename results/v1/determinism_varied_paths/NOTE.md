# Determinism test with a flaw that is itself a finding

The six repeats of each task ran in one run directory, so every repeat had its own trial id and therefore its own
working-directory path in the prompt (`/tmp/agent_opt_scratch/det_t0__<task>__00/`, `...__01/`). At temperature 0 the
repeats still differed from the first 30-70 characters on, in every task. A direct test against Ollama (same prompt six
ways: OpenAI endpoint and native API, temperature 0, 0.01, top_k 1, with and without a seed) gave byte-identical output
every time, so the stack is deterministic for an identical prompt; what changed the answers is **the path string in the
prompt**. The proper determinism test (`../determinism/`) uses byte-identical prompts.
