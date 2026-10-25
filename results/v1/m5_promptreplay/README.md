# M5.5 / H1: prompt-trimming replay (2026-10-24, 19:51-22:11; warm re-run 22:15-22:27)

**Question.** How much first-call prefill does a trimmed prompt save? Recorded call sequences of four tasks (captured in `m5_specdecode`) are replayed against Ollama (`qwen3.6-32k`, `max_tokens` 1, 3 repeats, variants interleaved) with
`drop_tools` of the four unused tool schemas (`task`, `todowrite`, `skill`, `webfetch`; "DROP4"), DROP4 plus the three unused MCP resource tools (`list_mcp_resource_templates`, `list_mcp_resources`, `read_mcp_resource`; "DROP7", only where they exist), and `strip_skills`.
`scripts/m5_promptreplay.sh`; per-task tables in `<task>/promptreplay.md`.

**Cold first call** (cache flushed by a nonce; the numbers that count):

| Task | prompt tokens default -> trimmed | first call default -> trimmed | whole sequence |
|---|---|---|---|
| repoqa_05 (3 calls) | 7395 -> 5422 (DROP4) | 30.3 s -> 22.3 s (-26 %) | x0.78 |
| bugfix_09 (13 calls) | 7419 -> 5446 | 30.4 s -> 22.3 s (-27 %) | x0.86 |
| comms_12 (2 calls) | 6691 -> 4718 (DROP4) / 4404 (DROP7) | 27.7 s -> 20.0 s / 18.4 s (-28 % / -34 %) | x0.77 / x0.72 |
| research_08e (4 calls, long web results) | 7684 -> 5711 (DROP4) / 5397 (DROP7) | 31.2 s -> 24.0 s / 22.7 s | x0.95 (later calls dominate) |

`strip_skills` removes only ~165 tokens and changes nothing (x0.97-1.02): the skills block of the isolated config is small.

* **H1 holds for cold sessions**: ~4 ms per prompt token removed, 26-34 % of the first call (8-12 s). The saving is a one-off per new session; long sequences dilute it (x0.95 on `research_08e`).
* **Warm**: the first run's warm columns are an artefact and must not be used - the replay's working-directory rewrite also changed the shared TMPDIR text (`/tmp/agent_opt_scratch/shared_tmp/opencode`) that sits in the bash tool description of every trial, so the prefix cache never matched (fixed in `harness/promptreplay.py`, test added). The re-run (`../m5_promptreplay_warm/`, 2 tasks, 2 repeats, `scripts/m5_promptreplay_warm.sh`): on `repoqa_05` the warm first call is 2.4-2.5 s with any prompt, and the **trimmed prompts are 6-15 % slower over the sequence** (as in the P7 probe, 4 %; unexplained, directional). On `comms_12` the warm replay still misses the cache (27 s = cold), so no warm statement is possible there; the cause (where the working directory sits relative to the tool block in the rendered prompt) is not resolved.
* **Verdict:** trimming unused tool schemas saves ~8-12 s per cold session and nothing when the cache is warm (it may cost a few percent); since back-to-back runs are mostly warm, it is worth doing only for new-session or cache-miss cases. Success impact not measured here (a trimmed agent would need an agent-config run).
