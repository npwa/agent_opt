# Server survey

Each configuration's private Ollama server was started once and read back (no trials). 'Experts on CPU' = layers whose MoE experts overflow to system memory (llama.cpp fit).

| Config | requested | n_ctx | KV (K/V) | KV MiB | flash attn | GPU layers | experts on CPU | model GPU MiB | model CPU MiB | VRAM used (fit) | size_vram | as requested |
|---|---|---:|---|---:|---|---:|---:|---:|---:|---:|---:|---|
| sw_q3_8b_32k | {"num_ctx": 32768, "base_model": "qwen3:8b"} | 32768 | f16/f16 | 4608.0 | auto | 33/37 | 0 | 4157.15 | None | 8501 | 8501 | yes |
| sw_q3_8b_32k_kvq8 | {"num_ctx": 32768, "base_model": "qwen3:8b", "kv_cache_type": "q8_0", "flash_attention": true} | 32768 | q8_0/q8_0 | 2448.0 | enabled | 37/37 | None | 4643.78 | None | None | 7284 | yes |
