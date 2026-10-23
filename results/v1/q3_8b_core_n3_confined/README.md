# qwen3:8b on the core suite, confined (first M5.4 screen, plan hypothesis H4)

`scripts/session_p2_p6.sh core` with `CORE_OUT=results/v1/q3_8b_core_n3_confined`: 14 core tasks × 3 trials, `qwen3:8b` with a
32k context and q8_0 KV cache on a private Ollama (all 37 layers on the GPU, P6 survey), isolated and confined OpenCode,
seed 1. It repeats `../q3_8b_core_n3/`, whose three `refactor_02` trials had escaped their working directory (that run is
kept, marked superseded). Compared with `qwen3.6` at 32k under identical conditions (`../m4_2/sw_32k_ref`, `COMPARISON_vs_qwen3.6_32k_ref.md`):

* **Success: much lower.** 10/42 = 24 % (CI 13-39 %) against 41/62 = 66 % (54-77 %): difference -42.3 pp, CI -57.1 to -23.1.
  Significant per task for `coding_01` (0/3 vs 4/5), `coding_03` (0/3 vs 5/5), `comms_12`, `logtriage_04` (0/3 vs 4/5); the only task
  where the small model does better is `coding_06` (3/3 vs 1/5, the word-limit task); `coding_05` and `refactor_02` are at par.
  The 8B model typically stops early or answers without doing the work (e.g. never writes the expected file).
* **Time: faster per trial, but the numbers are confounded.** Mean 83 s vs 147 s per trial, pooled median ratio 0.34. The reference
  runs carry the cold-prefix-cache bug (about +28 s per trial), the 8B run was made after the fix (32 of 42 first calls warm), so
  part of the gap is the bug; the rest is real: prefill is only 4.6 % of the time (fully GPU-resident), decode 85 %.
* **Verdict for H4:** a fully GPU-resident small model is faster per trial but not competitive on success for these tasks; the
  larger model's partial CPU offload costs time but buys a pass rate that is 2.7 times as high.
