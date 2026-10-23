# Nsight Systems alignment: baseline_32k__repoqa_05_retry_policy__00

66483 kernels and 8763 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 4.85 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **64.8 %**
- GPU-active share of LLM time: 9.2 % vs NVML utilisation 63.6 %
- Idle gaps between kernel-active intervals: n=69304, p50 0.00 ms, p99 19.00 ms, max 360.4 ms, total 43.71 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.8 | 16.4 | 90.0 | 0.66 | 2.63 |
| 2 | 10.7 | 14.3 | 46.7 | 0.92 | 3.46 |
| 3 | 8.0 | 3.0 | 54.1 | 0.01 | 2.68 |
