# First trim-tools run: skill-only trim (config bug, kept as a separate result)

80 trials, 8 tasks, n = 5, arms `sw_32k` (control) and `sw_32k_trimtools`, interleaved (21:03-22:00, 2026-10-27).

**Bug.** The trimmed arm was meant to remove `task`, `todowrite`, `skill` and `webfetch`. The runner set both OpenCode's legacy `tools` map
(from the task spec) and a permission deny; next to the permission rules the map conflicted and **only the `skill` tool (and the skills
paragraph) was removed** (first-call prompt 7 326 -> 7 023 tokens instead of -2k). Found by comparing first-call prompt sizes and confirmed with
a stub model server. With the global `permission: allow` the `tools` map is ignored (the control always received all 10 tools), so the task
specs' `tools:` lists never restricted anything in any run of this project; every arm of every comparison saw the same tool block.
Fixed in `harness/calibrate.py:build_config` (the map is dropped when tools are denied; test added) and re-run in `../m5_trimtools/`.

**What it still shows:** removing the `skill` tool alone changes nothing: pass 30/40 in both arms (difference 0 pp, CI -19 to +19), median time
31.2 s vs 30.4 s, one timeout (a 600 s single-step trial on `coding_02`) in the trimmed arm against none.
The control arm of this run is the control of `../m5_trimtools/`.
