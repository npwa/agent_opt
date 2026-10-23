# M4.2: does a 16k context window hurt? (run on 2026-10-22, 14:39-23:00)

Driver: `scripts/m4_2.sh`. Every trial ran confined (Landlock) on a private Ollama server, isolated OpenCode, seed 1,
shuffled order inside each configuration's block. No harness failures, no retries.

| Directory | What | Trials |
|---|---|---:|
| `sw_16k/` | `qwen3.6` at 16k context, all 18 tasks at n = 10, except `longctx_06`, `longctx_11`, `research_08e` (cannot fit in 16k) at n = 3 | 159 |
| `sw_32k_ref/` | the same server and confinement at 32k: n = 5 (n = 3 for the long tasks and for `c_ringbuf_03`, `coding_10`) | 80 |
| `sw_32k_tmpfix/` | a later check of a bug found during this run (see below): six cheap tasks at 32k with the fixed runner, n = 4 | 24 |
| `COMPARISON_*.md` | 16k vs the 32k reference, 16k vs the earlier isolated baseline, the 32k reference vs the isolated baseline | |

## Result

* **Success: no significant overall change, but a clear loss where the context outgrows 16k.** 16k passes 104/159 = 65 %
  (CI 58-72 %), the 32k reference 58/80 = 72 % (62-81 %); difference -7.1 pp, CI -18.6 to +5.6 (Newcombe).
  Clear per-task losses: `logtriage_04` 1/10 vs 4/5 (-70 pp, CI -88 to -18), `longctx_06` 0/3 vs 3/3 (-100 pp, CI
  -100 to -21), `longctx_11` 0/3 vs 2/3 (-67 pp, CI -94 to +6). Everything else is within noise at these sample sizes.
* **Why `logtriage_04` fails at 16k:** its prompts reach 13.2-15.6k tokens in a 16 384-token window, so the oldest tool
  results are dropped and the agent loses facts it already found (answers miss the start time, first host or culprit);
  at 32k the same task reaches 19-26k tokens and passes. The rule of thumb is "tasks whose working context passes about
  14k tokens degrade at 16k".
* **The v0 observation "16k regresses on fizzbuzz-style tasks" is not reproduced:** `coding_02` passes 10/10 at 16k (5/5 at 32k).
* **Time:** the pooled per-task median ratio 16k/32k is 0.94 (CI 0.90-0.98, p = 0.02), mean 114 s vs 133 s per trial;
  prefill is 42 % of the time in both, decode 56 % vs 53 %. Part of the difference is the long tasks failing early, so
  16k is not "faster" in a useful sense.
* **Server effect on VRAM (P4 survey):** 16k moves one expert layer to the GPU (29 instead of 30 on the CPU), a small gain
  in decode that this data cannot resolve.
* **Verification claims (`sw_16k/claims.md`, `sw_32k_ref/claims.md`):** 16k: 39 trials end with a claim in 80 code-producing
  trials, 4 of them unsupported (2 unverified, 1 stale, 1 contradicted = 10 %), false claims 8/80 = 10 %; 32k reference: 17
  claims in 36 trials, none unsupported, false claims 6/36 = 17 %. Over-claiming after a weak self-check is the pattern
  (P1), unaffected by the context size within the noise.
* **Confinement and the private server do not change success:** the 32k reference passes 49/64 = 77 % on the 14 original
  tasks vs 99/140 = 71 % in the earlier unconfined baseline on the system server (+5.8 pp, CI -7.7 to +17.7). Their
  *timing* is not comparable: see the next section.

## A bug found during the run (affects time, not success)

All first calls were cold (0 of 239 warm; the isolated baseline had 130 of 140 warm): OpenCode prints `$TMPDIR` in the
description of its first tool (`bash`), the model server renders tools at the very start of the prompt, and the confinement
had given every trial its own `TMPDIR`, so each prompt differed from character ~570 on and the cross-trial prefix cache
never hit (+28 s per trial). Found by comparing captured requests with a stub server; fixed in the runner (one constant
shared temp directory, emptied before each trial, inside the confinement; test added). The check `sw_32k_tmpfix/` confirms
it: 22 of 24 first calls are warm again (median 2.5 s instead of 30 s), mean time per trial 41 s instead of 64 s on the same
six tasks (ratios per task 0.31-0.68), pass rates unchanged within noise (20/24 vs 19/25). **Consequences:** the 16k vs 32k
comparison is fair (both ran with the same handicap) and success is unaffected; absolute times and the comparisons of
`sw_32k_ref` / `sw_16k` with the isolated baseline are inflated by the cold first calls (the ratio 1.61 in
`COMPARISON_32k_ref_vs_isolated_baseline.md` is this bug, not a cost of confinement); future runs have the fix.
