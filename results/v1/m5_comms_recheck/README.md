# M5.5 / H6: does the `comms` agent still cut model calls? (2026-10-24, 16:55-19:14)

**Question.** The original pilot found that the `comms` agent (answer in the same turn as the tool call) cut model calls 36 -> 12 (n = 1-2). Does that hold at n = 10 under the current harness?

**Design.** `sw_32k` (tasks use the `comms` agent) against `sw_32k_nocomms` (`agents: {comms: default}`, the built-in agent) on `comms_07_confirm_send` and `comms_12_timezone_slot`, n = 10 per cell, 40 trials, seed 3, same private server, confined (`scripts/m5_comms_recheck.sh`). Calls = steps in `trace.json`.

| Task | Arm | Calls, median (range) | Pass | Median time s |
|---|---|---|---:|---:|
| comms_07 | comms agent | 11 (10-12) | 10/10 | 95.8 |
| comms_07 | default agent | 12 (11-16) | 9/10 | 89.2 |
| comms_12 | comms agent | 2.5 (2-9) | 3/10 | 273.2 |
| comms_12 | default agent | 3 (2-8) | 6/10 | 190.1 |

Pooled: pass 13/20 = 65 % against 15/20 = 75 % (default - comms +10 pp, Newcombe -18 to +36); timeouts 3 (comms agent, all `comms_12`) against 0; time ratio default/comms 0.83 (CI 0.63-1.04, p = 0.086, inconclusive).

* **The call reduction is not reproduced.** The default agent needs about one call more on `comms_07` (12 against 11) and half a call more on `comms_12`; there is no 3x reduction. The pilot's 36 -> 12 compared different setups (n = 1-2, other harness version, cold-cache effects) and should not be quoted.
* **No success benefit, a possible cost on `comms_12`** (3/10 against 6/10, intervals overlap) with the 3 timeouts.
* Time: the comms arm is not faster; the default arm is nominally faster (inconclusive).

**Verdict.** The `comms` agent gives at most a call or so fewer on `comms_07` and no measurable benefit; the original claim is withdrawn. Per-task tables, decomposition: `RESULTS.md`, `report.html`.
