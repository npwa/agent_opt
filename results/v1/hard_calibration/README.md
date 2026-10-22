# Calibration of the four hard tasks (plan item P5)

Each task was run 10 times on `baseline_32k` (isolated OpenCode, seed 1, shuffled) to see whether it lands in the
30-70 % pass band that leaves room to show improvements. Final numbers:

| Task | n | passed | rate (95 % Wilson) | timeouts | median time | max time |
|---|---:|---:|---|---:|---:|---:|
| bugfix_09_booking_slots | 10 | 3 | 30 % [11, 60] | 0 | 100 s | 223 s |
| coding_10_wcx_constrained | 10 | 4 | 40 % [17, 69] | 3 | 284 s | 600 s |
| comms_12_timezone_slot | 10 | 6 | 60 % [31, 83] | 0 | 134 s | 251 s |
| longctx_11_harbor_report | 10 | 6 | 60 % [31, 83] | 0 | 133 s | 157 s |

Where the files are: `hard_calibration/` holds `coding_10`, `comms_12` and `longctx_11` (their final specs) and the
**superseded** `bugfix_09` trials; `../hard_calibration_bugfix_09_v3/` holds the final 10 `bugfix_09` trials. The
`bugfix_09` trials in this directory used the first version of the fixture and prompt and must not be used.

## How the tasks were tuned (everything is kept honest on purpose)

* `bugfix_09_booking_slots` (6 bugs, 3 of them visible in the failing tests): first version 0/10, because the visible
  tests exposed only two bugs and agents stopped there (7 of 10 left 4 bugs in). A stronger prompt ("check every function
  against its docstring") gave 1/9, and exposed a **mistake in the fixture**: a visible test asserted the buggy behaviour
  of the trailing free gap, so agents that fixed it saw a failing test and believed they had broken something. After
  correcting that test (the trailing-gap bug now shows up as a third failing visible test) the result is 3/10. The bugs
  still left unfixed are almost always the double-counted overlapping minutes in `total_busy_minutes` (7 of 7 failures).
* `coding_10_wcx_constrained`: the first smoke run timed out at 900 s because the prompt told the agent to test edge
  cases itself and it deliberated until the limit, even though its file passed the hidden checks. The self-test
  sentence was removed and the timeout set to 600 s; 3 of 10 trials still hit it. Typical wrong answers: counting the
  unterminated last line as a line, or forgetting it for the longest-line length.
* `comms_12_timezone_slot` and `longctx_11_harbor_report` needed no change. The common wrong slot is Tuesday 14:30 UTC (only
  30 minutes free); the common wrong answers in the report task apply the uncorrected figures or miss the errata.

All four are in the `core` suite. One caveat: n = 10 per task gives wide intervals (30 % is [11, 60] %), so these are
"in the band", not "known to be at 30 %"; the band will be re-checked as a by-product of the first sweep runs.
