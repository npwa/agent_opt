# Nsight Systems alignment: baseline_32k__refactor_02_rename_price__00

93253 kernels and 55255 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 13.35 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **96.7 %**
- GPU-active share of LLM time: 12.8 % vs NVML utilisation 49.8 %
- Idle gaps between kernel-active intervals: n=142566, p50 0.00 ms, p99 17.68 ms, max 469.4 ms, total 89.06 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.7 | 17.3 | 82.9 | 0.68 | 2.56 |
| 2 | 8.5 | 17.8 | 43.4 | 0.67 | 2.45 |
| 3 | 7.4 | 17.0 | 46.5 | 0.67 | 2.52 |
| 4 | 8.5 | 17.6 | 35.5 | 0.68 | 2.46 |
| 5 | 11.8 | 11.2 | 53.7 | 0.77 | 11.96 |
| 6 | 5.8 | 11.5 | 36.8 | 0.68 | 2.85 |
