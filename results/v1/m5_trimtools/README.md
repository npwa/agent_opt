# H1 success guard: removing four unused tool schemas

Question: the prompt replay (`../m5_promptreplay/`) showed that dropping unused tool schemas makes a cold first call 26-34 % faster.
Does it change the pass rate? Config `sw_32k_trimtools` (`deny_tools: [task, todowrite, skill, webfetch]`) against the control
`sw_32k`, eight short non-MCP tasks, n = 5 each (40 trials per arm), seed 5, same private server settings. Script: `scripts/m5_trimtools.sh`.

**Design note.** The control arm is the `sw_32k` arm of `../m5_trimtools_skill_only/` (run 21:03-22:00 on 2026-10-27, interleaved with a
trimmed arm that turned out to remove only the `skill` tool, see that README). The corrected trimmed arm was run afterwards (22:07-22:32),
so the two arms are **not interleaved** (a drift between the two runs would not be balanced out). Prompt diff: `prompt_diff/PROMPT_DIFF.md`.

| | control | trimmed (4 tools) |
|---|---:|---:|
| first-call prompt tokens (median) | 7 326 | 5 196 (-2.1k) |
| pass rate | 30/40 = 75 % [60, 86] | 27/40 = 68 % [52, 80] |
| difference | | **-7.5 pp, CI -26 to +12** |
| median time per trial | 30.4 s | 29.7 s (ratio 0.98, CI 0.78-1.27, p = 0.96) |
| timeouts | 0 | 0 |
| median first-call time to first token | 2.8 s | 2.6 s |

Per task (control / trimmed, of 5): bugfix_01 5/5, coding_01 3/1, coding_02 4/5, coding_03 4/4, coding_05 5/5, coding_06 3/0, refactor_02 4/4, repoqa_05 2/3.
The two tasks with a drop (`coding_01`, `coding_06`) are the least reliable ones (5/12 and 3/12 in the isolated baseline); at n = 5 a swing of
that size is within the noise.

**Reading.** The trimmed prompt is really 2.1k tokens smaller (checked in the proxy logs and with a stub server), per-trial time is unchanged
once the cache is warm (as the warm replay predicted), and the pass rate shows **no significant change but a loss of up to about 26 pp is not
excluded** (n = 40 per arm, non-interleaved arms). So the cold-start saving (about 8-12 s per new session) is real, but the success side
of the guard is inconclusive; confirming "no loss" would need roughly n = 20 per task on the tasks that moved.
