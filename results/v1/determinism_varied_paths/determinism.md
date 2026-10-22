# Determinism: determinism

Same task and configuration repeated at temperature 0. 'Identical' = same text parts and tool calls, in order.

| Task | repeats | identical to the most common run | distinct runs | first divergence at step | calls with equal token count | verdicts agree |
|---|---:|---:|---:|---|---:|---|
| bugfix_01_moving_average | 6 | 1 | 6 | 0, 0, 0, 6, 7 | 10/26 (38 %) | yes |
| coding_05_scientific_lan | 6 | 1 | 6 | 0, 0, 0, 0, 0 | 0/5 (0 %) | yes |
| coding_06_tax_rates_lan | 6 | 1 | 6 | 0, 0, 0, 0, 0 | 0/5 (0 %) | no |
| repoqa_05_retry_policy | 6 | 1 | 6 | 0, 1, 1, 1, 1 | 4/15 (27 %) | yes |

Over all tasks 27 % of compared calls have the same completion-token count as the reference repeat; the plan's rule (>= 90 %) is NOT met.
