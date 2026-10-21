# Speculative decoding groundwork (plan item P9)

Three questions: does a llama.cpp build exist that we control, which models could be a draft for `qwen3.6`, and does
speculation run at all on this box before M5.3 measures it? Everything here was done on 2026-10-21.

## 1. The two llama.cpp engines

| | project-local mainline build | llama-server bundled with Ollama |
|---|---|---|
| where | `~/work/agent_opt_ad/llama.cpp/build-cuda` (commit b2f036c0f, build 11562, CUDA 12.0 `nvcc`, arch 86, flash attention on) | `/usr/local/lib/ollama/llama-server` (build b4d6c7d8f; recorded in `platform.yaml` as `ollama_llama_server`) |
| tools | `llama-server`, `llama-cli`, `llama-bench`, `llama-tokenize`, `llama-speculative-simple` | `llama-server` |
| loads the Ollama `qwen3.6` blob | **no**: `key qwen35moe.rope.dimension_sections has wrong array length; expected 4, got 3` (`mainline_failure/`) | yes: it logs "detected Ollama-format qwen35moe GGUF; applying compatibility fixes" |
| speculation types (`--spec-type`) | none, draft-simple, draft-eagle3, draft-mtp, draft-dflash, draft-dspark, ngram-* | none, draft-simple, draft-eagle3, draft-mtp, draft-dflash, ngram-* |

So the engine for every experiment on this model is Ollama's own `llama-server`, the one the baselines ran on. The
mainline build works for other models and for tooling (`llama-tokenize`, `llama-bench`), and could run the Ollama blob
only after its metadata is converted (not done). **Trap:** started directly, the bundled server silently loads only its
CPU backend (12.7 tok/s instead of 36); it needs `LD_LIBRARY_PATH=/usr/local/lib/ollama:/usr/local/lib/ollama/cuda_v13`
**and** `GGML_BACKEND_PATH=/usr/local/lib/ollama/cuda_v13/libggml-cuda.so` (see `smoke.py`).

## 2. Which model could be the draft? (`vocab_check.md`)

`python -m harness.vocabcheck` applies llama.cpp's own compatibility rule (`harness/ggufmeta.py`, an isolated
stdlib-only GGUF reader) to the target and every installed model plus the small `qwen3.5` models of the Ollama
registry (only the first 64 MB of each file were fetched, never the weights).

* Target `qwen3.6-32k`: architecture `qwen35moe`, GPT-2/BPE tokenizer with the `qwen35` pre-tokenizer, 248 320 tokens.
* **`qwen3:8b`, the planned second model, is not a draft for it**: 151 936 tokens and the `qwen2` pre-tokenizer. The same
  holds for every other installed non-`qwen3.6` model (`qwen2.5`, `llama3`, `mistral`, `gemma3`, `phi4`, `deepseek`).
* **`qwen3.5:0.8b / 2b / 4b / 9b` share the target's tokenizer** (type, pre-tokenizer, 248 320 tokens, every id below
  248 070 identical) but llama.cpp still refuses them: the last 250 reserved ids differ (`[PAD0]` ... in the target,
  `<|audio_start|>`, `<tts_pad>` ... in the draft). They are never produced in text, so a draft whose token *texts* for those
  ids are overwritten in a copy of its GGUF would be accepted; that is a metadata rewrite plus a file copy, and it is
  only worth doing if the built-in alternative below fails.
* **The target has its own draft: 20 multi-token-prediction tensors** (`mtp.*` in the blob, `blk.40.nextn.*` in
  llama.cpp), so `--spec-type draft-mtp` needs no second model and no extra VRAM.

## 3. Smoke test (`smoke.py`, `smoke.json`, logs)

One greedy completion of 300 tokens per output kind and speculation type on the bundled `llama-server`
(32k context, one slot, flash attention on, default draft settings). `code` = new C code, `prose` = an essay, `copy` = a
file rewritten with one comment added (output that mostly repeats the context, like the agent's write/edit calls).
One run each: a feasibility check, not a benchmark.

| Spec type | Output | tok/s | vs none | drafted | accepted | greedy text identical to `none` |
|---|---|---:|---:|---:|---:|---|
| none | code | 35.5 | +0 % | - | - | yes |
| none | prose | 37.0 | +0 % | - | - | yes |
| none | copy | 37.2 | +0 % | - | - | yes |
| draft-mtp | code | 33.9 | -4 % | 357 | 179 (50 %) | yes |
| draft-mtp | prose | 28.3 | -23 % | 470 | 142 (30 %) | **no** |
| draft-mtp | copy | 39.4 | +6 % | 237 | 142 (60 %) | yes |
| ngram-mod | code | 33.9 | -5 % | - | - | yes |
| ngram-mod | prose | 33.8 | -8 % | - | - | yes |
| ngram-mod | copy | 33.5 | -10 % | 192 | 159 (83 %) | yes |
| ngram-map-k | code | 33.2 | -6 % | 48 | 1 (2 %) | yes |
| ngram-map-k | prose | 36.8 | -0 % | - | - | yes |
| ngram-map-k | copy | 32.9 | -12 % | 192 | 165 (86 %) | **no** |

What it shows:
* Every speculation type loads and runs with the Ollama blob; decode without speculation is 35-37 tok/s.
* **No type is faster at default settings** (-23 % to +6 %). Acceptance is high where the output repeats the context
  (ngram 83-86 %, MTP 60 %), yet the speed does not follow: the model is a 35B mixture of experts whose experts mostly
  stay in system memory (P4 survey), and a verification batch of many drafted tokens touches many more experts than a
  single-token step, so the weights must be read from RAM for the whole batch.
* **Greedy output is not guaranteed to be identical**: MTP on prose and ngram-map-k on the copy task produced different
  text from the run without speculation (batched verification changes the floating-point results enough to flip a near-tied
  choice). The M5.3 acceptance rule "token-identical before any timing is trusted" fails for them as configured.

## Consequences for M5.3

1. The draft-model plan (`qwen3:8b`) is replaced by built-in speculation: `draft-mtp` and `ngram-mod` / `ngram-map-k`
   (and optionally a patched `qwen3.5:0.8b`).
2. The sweep is about draft length, not about which model: small `--spec-draft-n-max` (2-4) for MTP and a short
   `--spec-ngram-mod-n-max` (8-16, default 64) keep the verification batch small; test on replayed agent outputs
   (write/edit calls repeat file contents) rather than on one prompt.
3. Report output equivalence for every setting, and treat a setting that changes greedy output as a different model
   configuration, not as a free speed-up.
