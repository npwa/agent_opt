# Nsight Systems alignment: baseline_32k__coding_01_my_sqrt_review__00

60186 kernels and 27042 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 7.36 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **89.0 %**
- GPU-active share of LLM time: 11.2 % vs NVML utilisation 49.6 %
- Idle gaps between kernel-active intervals: n=81286, p50 0.00 ms, p99 18.56 ms, max 398.9 ms, total 54.84 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.7 | 15.5 | 92.0 | 0.68 | 2.65 |
| 2 | 4.9 | 13.6 | 32.6 | 0.85 | 3.49 |
| 3 | 8.1 | 14.1 | 24.3 | 0.73 | 2.83 |
