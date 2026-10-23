# Nsight Systems alignment: baseline_32k__repoqa_05_retry_policy__00

63998 kernels and 9514 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 4.89 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **67.4 %**
- GPU-active share of LLM time: 9.3 % vs NVML utilisation 66.5 %
- Idle gaps between kernel-active intervals: n=67570, p50 0.00 ms, p99 19.09 ms, max 368.8 ms, total 42.70 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.8 | 16.6 | 91.8 | 0.67 | 2.66 |
| 2 | 8.6 | 17.0 | 55.5 | 0.68 | 2.56 |
| 3 | 8.0 | 2.9 | 52.2 | 0.00 | 2.69 |
