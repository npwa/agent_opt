# M5.3 speculative decoding on replayed agent calls (2026-10-23)

**Question (H5):** does speculative decoding speed up the agent's decode phase on this box, and does it keep the output?

**Method.** `scripts/m5_specdecode.sh capture` ran one trial of 10 diverse tasks through the proxy with request capture (`capture/`, 51 model calls, 8 of 10 trials passed).
`python -m harness.specsweep run` replays those calls in order against Ollama's bundled `llama-server` (the engine of the baselines,
32k context, one slot, flash attention on, `--jinja`), greedy (temperature 0, seed 1, at most 1500 new tokens per call), once per
speculation setting; `report` summarises. Each reply is classified by what the model produced: `write`/`edit` tool call
(`tool_code`, 14 calls), any other tool call (`tool_other`, 28) or prose (7). `none_repeat` is the run without speculation done a
second time: the floor for "identical". Two calls (about 44k tokens when this server renders them, over the 32k context; Ollama had
truncated them in the original runs) fail in every setting and are left out: 49 calls are compared. `calls.jsonl` has every call.

| Setting | tok/s (pooled) | Speed vs none | Drafted | Accepted | Acceptance | Replies identical to none |
|---|---:|---:|---:|---:|---:|---|
| none | 37.8 | 1.00 | - | - | - | 49/49 |
| none_repeat | 38.1 | 1.01 | - | - | - | 49/49 |
| MTP, draft length 2 | 38.1 | 1.01 | 15087 | 8541 | 57 % | 14/49 [18 %, 42 %] |
| MTP, draft length 3 | 34.6 | 0.92 | 23032 | 10482 | 46 % | 16/49 |
| MTP, draft length 4 | 30.2 | 0.80 | 27776 | 10271 | 37 % | 16/49 |
| ngram-mod, length 8 | 34.6 | 0.92 | 2992 | 2514 | 84 % | 40/49 [69 %, 90 %] |
| ngram-mod, length 16 | 39.4 | 1.04 | 3712 | 2500 | 67 % | 36/49 [60 %, 84 %] |
| ngram-mod, length 64 (default) | 31.3 | 0.83 | 9011 | 2465 | 27 % | 37/49 |
| ngram-map-k | 28.9 | 0.77 | 9978 | 3174 | 32 % | 35/49 [58 %, 82 %] |

Per output type see `sweep/RESULTS.md` (`summary.json`).

**Findings.**
1. **No setting is reliably faster.** MTP at length 2 (1.01x) equals the repeat run; `ngram-mod` at length 16 (1.04x; 1.07x on `write`/`edit` calls) is the only one above
   the +/-1 % noise of the repeated reference, from 49 calls of 10 tasks: not a result to build on. Longer drafts are slower (MTP 0.92x at 3, 0.80x at 4).
2. **Acceptance does not turn into speed.** `ngram-mod` at length 8 accepts 84 % of its drafts and is 8 % slower. The experts mostly sit in system memory (P4), so a
   verification batch of several tokens reads many more expert weights than a one-token step.
3. **Speculation changes the generated text.** The run without speculation reproduces itself exactly (49/49), so the differences come from the setting: MTP changes
   about two thirds of the replies (14-16 of 49 identical), the n-gram types 18-27 %. By the rule fixed in P9, every such setting is a different model
   configuration and not a free speed-up.
4. Consistent with the P9 smoke test (one prompt per output kind, no speed-up at default settings).

**Verdict H5: not supported** on this hardware and model; keep speculation off. Limits: one trial per task, 49 compared calls (wide intervals), greedy decoding
(the agent samples at temperature 1.0), llama-server's chat rendering instead of Ollama's, one engine build (b4d6c7d8f).
