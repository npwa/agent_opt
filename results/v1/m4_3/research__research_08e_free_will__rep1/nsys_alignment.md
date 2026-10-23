# Nsight Systems alignment: baseline_32k__research_08e_free_will__00

445517 kernels and 107298 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 49.73 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **97.9 %**
- GPU-active share of LLM time: 11.9 % vs NVML utilisation 67.6 %
- Idle gaps between kernel-active intervals: n=546873, p50 0.00 ms, p99 18.56 ms, max 1896.2 ms, total 369.77 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.9 | 16.4 | 77.6 | 0.70 | 2.53 |
| 2 | 9.7 | 16.8 | 72.6 | 0.70 | 2.60 |
| 3 | 9.4 | 18.0 | 48.4 | 0.67 | 2.37 |
| 4 | 10.2 | 18.3 | 81.7 | 0.68 | 2.46 |
| 5 | 8.7 | 19.1 | 50.7 | 0.67 | 2.34 |
| 6 | 10.4 | 19.0 | 79.3 | 0.68 | 2.44 |
| 7 | 10.2 | 18.0 | 92.5 | 0.70 | 2.53 |
| 8 | 10.2 | 18.6 | 83.1 | 0.70 | 2.57 |
| 9 | 5.6 | 16.1 | 22.4 | 0.72 | 2.66 |
