# M5.5 / H6: does a verification-step coding agent help? (2026-10-23, 20:01-23:32)

**Question.** A `coding` agent that must run its code, test an edge case and report exactly what it ran: does it raise the pass rate and
lower false "verified" claims?

**Design.** Three arms on the same private server (32k, confined, trials interleaved), nine code-writing core tasks, n = 3 each, 81 trials
(`scripts/m5_agents.sh`, configs `sw_32k`, `sw_32k_lean`, `sw_32k_verify`; the new config key `agents:` maps the task's agent to another one).
A smoke run found that **any custom OpenCode agent replaces the built-in 9.6k-character system prompt with its own text** (1.8k characters
for `coding`), so the verification agent differs from the default in two ways. The `lean` arm (`agent-config/agent/coding_lean.md`, one
sentence, no verification text) separates them: lean = shorter prompt only, verify = shorter prompt + verification text.

| Arm | Pass rate (95 % Wilson) | Cells all pass / all fail / mixed | Timeouts | Final "I ran it" claims | False claims |
|---|---|---|---:|---:|---:|
| sw_32k (default agent) | 15/27 = 56 % [37, 72] | 3 / 1 / 5 | 3 | 8 | 3 |
| sw_32k_lean | 16/27 = 59 % [41, 75] | 3 / 1 / 5 | 2 | 8 | 5 |
| sw_32k_verify | 20/27 = 74 % [55, 87] | 5 / 1 / 3 | 0 | 13 | 1 |

Differences (Newcombe 95 %): verify - default +18.5 pp [-6.7, +40.8]; verify - lean +14.8 pp [-10.0, +37.3]; lean - default +3.7 pp [-21.3, +28.1].

* **Prompt shortening alone does nothing** (lean = default within noise): the 9.6k-character system prompt can be replaced without losing success.
* **Verification text: a tendency, not a result.** The gain comes from `bugfix_09` (0/3 -> 2/3), `c_ringbuf_03` (1/3 -> 3/3) and `coding_10` (1/3 -> 2/3; lean also 2/3);
  it loses on `coding_06` (1/3 -> 0/3). The agent runs and reports more (13 final claims against 8) and only one claim is false (3 and 5 in the other arms).
  Cells hold only 3 trials, so every interval includes zero.
* **Time:** sum of task medians 2024 s (default), 918 s (lean), 1076 s (verify); wide overlapping intervals. Verify is slower where it adds checks
  (`coding_03` 209 s against 50-67 s) but has no timeouts; the default timed out three times on the long tasks. No conclusion on speed.
* Not done: the re-check of the `comms` agent's call reduction (planned in M5.5).

Per-task table, decomposition, claims and consistency: `RESULTS.md`, `claims.md`, `consistency.md`, `report.html`.
