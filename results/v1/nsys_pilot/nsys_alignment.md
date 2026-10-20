# Nsight Systems alignment: baseline_32k__bugfix_01_moving_average__00

87256 kernels and 33490 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 10.10 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **91.8 %**
- GPU-active share of LLM time: 10.0 % vs NVML utilisation 49.9 %
- Idle gaps between kernel-active intervals: n=114804, p50 0.00 ms, p99 20.94 ms, max 423.5 ms, total 85.23 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.0 | 15.4 | 88.9 | 0.71 | 2.72 |
| 2 | 7.1 | 15.4 | 33.4 | 0.74 | 2.82 |
| 3 | 7.4 | 15.0 | 45.8 | 0.78 | 2.96 |
| 4 | 5.7 | 11.3 | 33.2 | 0.82 | 3.83 |
| 5 | 0.0 | 0.0 | 48.2 | n/a | n/a |
