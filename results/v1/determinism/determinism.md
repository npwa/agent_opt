# Determinism: determinism

Same task and configuration repeated at temperature 0. 'Identical' = same text parts and tool calls, in order.

| Task | repeats | identical to the most common run | distinct runs | first divergence at step | calls with equal token count | verdicts agree |
|---|---:|---:|---:|---|---:|---|
| bugfix_01_moving_average | 6 | 6 | 1 | - | 25/25 (100 %) | yes |
| coding_05_scientific_lan | 6 | 6 | 1 | - | 5/5 (100 %) | yes |
| coding_06_tax_rates_lan | 6 | 6 | 1 | - | 5/5 (100 %) | yes |
| repoqa_05_retry_policy | 6 | 4 | 3 | 4, 4 | 11/15 (73 %) | no |

Over all tasks 92 % of compared calls have the same completion-token count as the reference repeat; the plan's rule (>= 90 %) is met.
