# Nsight Systems alignment: baseline_32k__research_08e_free_will__00

160406 kernels and 58976 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 20.28 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **98.5 %**
- GPU-active share of LLM time: 12.4 % vs NVML utilisation 74.2 %
- Idle gaps between kernel-active intervals: n=213440, p50 0.00 ms, p99 18.33 ms, max 1489.5 ms, total 143.33 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.9 | 14.8 | 78.8 | 0.75 | 3.28 |
| 2 | 9.5 | 17.9 | 81.0 | 0.67 | 2.36 |
| 3 | 10.3 | 17.5 | 62.7 | 0.71 | 2.59 |
