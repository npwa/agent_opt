# M5.5 / H6 confirmation: does the verification-step agent raise the pass rate? (2026-10-24, ~08:50-15:43)

**Question.** The n = 3 screen (`results/v1/m5_agents/`) showed verify 74 % against default 56 %, driven by four tasks that moved. Does the gain hold with n = 10?

**Design.** `sw_32k` (default agent) against `sw_32k_verify` on `bugfix_09_booking_slots`, `c_ringbuf_03_ringbuffer`, `coding_10_wcx_constrained`, `coding_06_tax_rates_lan`; n = 10 per cell, 80 trials, interleaved, same private server, confined (`scripts/m5_agents_confirm.sh`, seed 2, screen trials ignored).

| Arm | Pass rate (95 % Wilson) | Timeouts | Final claims / false |
|---|---|---:|---|
| sw_32k | 19/40 = 48 % [33, 63] | 0 | 17 / 7 |
| sw_32k_verify | 18/40 = 45 % [31, 60] | 5 | 17 / 2 |

Pass-rate difference verify - default: -2.5 pp (Newcombe 95 % -23 to +19). False claims (30 applicable trials each): -16.7 pp (-35 to +2).

| Task | default | verify |
|---|---:|---:|
| bugfix_09 | 4/10 | 6/10 |
| c_ringbuf_03 | 8/10 | 8/10 |
| coding_10 | 5/10 | 3/10 |
| coding_06 | 2/10 | 1/10 |

* **The screen's gain did not replicate.** The four tasks were chosen because they moved at n = 3, so the screen overstated the effect (regression to the mean). Pooled with the screen on these tasks: 25/52 against 22/52 (+5.8 pp, CI -13 to +24).
* **Fewer false "verified" claims: suggestive only.** Same number of final claims, 2 false against 7; the interval reaches zero.
* **Cost: timeouts.** Verify timed out 5 times (4 on `coding_10`, 1 on `c_ringbuf_03`), default never; it spends the time budget on extra checks. Sum of task medians 729 s against 641 s, intervals overlap.
* Every cell is mixed in both arms (outcomes are close to coin flips on these tasks).

**Verdict.** H6 is not confirmed for pass rate. Keep the default agent unless false claims matter more than long-task timeouts. Measuring the false-claim effect needs a larger run on tasks where claims occur.

Per-task table, decomposition, claims, consistency: `RESULTS.md`, `claims.md`, `consistency.md`, `report.html`.
