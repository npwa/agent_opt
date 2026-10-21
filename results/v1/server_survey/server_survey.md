# Server survey

Each configuration's private Ollama server was started once and read back (no trials). 'Experts on CPU' = layers whose MoE experts overflow to system memory (llama.cpp fit).

| Config | requested | n_ctx | KV (K/V) | KV MiB | flash attn | GPU layers | experts on CPU | model GPU MiB | model CPU MiB | VRAM used (fit) | size_vram | as requested |
|---|---|---:|---|---:|---|---:|---:|---:|---:|---:|---:|---|
| sw_32k | {"num_ctx": 32768} | 32768 | f16/f16 | 640.0 | auto | 41/42 | 30 | 6519.57 | 15445.79 | 7529 | 7778 | yes |
| sw_16k | {"num_ctx": 16384} | 16384 | f16/f16 | 320.0 | auto | 41/42 | 29 | 6871.01 | 15094.36 | 7545 | 7793 | yes |
| sw_8k | {"num_ctx": 8192} | 8192 | f16/f16 | 160.0 | auto | 41/42 | 29 | 7017.57 | 14947.79 | 7531 | 7780 | yes |
| sw_64k | {"num_ctx": 65536} | 65536 | f16/f16 | 1280.0 | auto | 41/42 | 31 | 5875.01 | 16090.36 | 7557 | 7805 | yes |
| sw_32k_kvq8 | {"num_ctx": 32768, "kv_cache_type": "q8_0", "flash_attention": true} | 32768 | q8_0/q8_0 | 340.0 | enabled | 41/42 | 29 | 6871.01 | 15094.36 | 7581 | 7829 | yes |
| sw_32k_fa_off | {"num_ctx": 32768, "flash_attention": false} | 32768 | f16/f16 | 640.0 | disabled | 41/42 | 31 | 5730.45 | 16234.92 | 7562 | 7563 | yes |
| sw_32k_fit512 | {"num_ctx": 32768, "fit_target_mib": 512} | 32768 | f16/f16 | 640.0 | auto | 41/42 | 29 | 6726.45 | 15238.92 | 7728 | 7729 | yes |
