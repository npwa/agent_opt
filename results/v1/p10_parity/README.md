# P10: instrumentation parity top-up (2026-10-24, 08:50-10:21)

**Question.** Does measuring through the metrics proxy change what is measured? Same run, same private server (`sw_32k`, 32k, confined),
instrumented trials (through the proxy) and uninstrumented trials (OpenCode straight to Ollama) interleaved, 96 of each
(`scripts/p10_parity.sh`, `--parity-fraction 1.0`). Low-variance tasks only: `coding_05`, `coding_06` (time CV 0.14-0.18 in earlier runs) n = 30 per arm,
`coding_01`, `repoqa_05`, `refactor_02` n = 12 per arm. 192 trials, no harness failures.

| Task | Trials per arm | Median instrumented (s) | Median uninstrumented (s) |
|---|---:|---:|---:|
| coding_05_scientific_lan | 30 | 11.5 | 11.9 |
| coding_06_tax_rates_lan | 30 | 14.1 | 14.0 |
| coding_01_my_sqrt_review | 12 | 36.5 | 37.0 |
| repoqa_05_retry_policy | 12 | 30.8 | 29.2 |
| refactor_02_rename_price | 12 | 73.8 | 71.9 |

**Result (`parity.md`): INCONCLUSIVE by the pre-set rule.** Time ratio uninstrumented/instrumented 1.042 (95 % CI 0.998-1.094, n = 96 pairs) against the
+/-5 % tolerance; success 58 % instrumented vs 65 % uninstrumented (intervals overlap).

* The direct path is not faster, so there is no sign of proxy overhead; the point estimate leans the other way and the per-task medians agree within 2-7 %.
* The upper end of the interval (9 %) is set by a few long trials per task, which move the pooled ratio; the three parity measurements so far are 1.06, 0.87 and 1.04.
  The overhead is therefore bounded to about +/-5-9 %, not shown to be below 5 %.
* The rule was not changed after seeing the data. A median-based variant (resistant to long trials) would be a follow-up decision.
* Two `refactor_02` trials were flagged by the decomposition (unattributed time >= 5 %); they were not examined.
* This run replaces the earlier parity checks (`baseline`, `baseline_isolated`), which compared against older runs with the prefix-cache bug.
