# Nsight Systems alignment: baseline_32k__comms_07_confirm_send__00

101067 kernels and 100661 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 19.72 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **97.2 %**
- GPU-active share of LLM time: 13.2 % vs NVML utilisation 38.7 %
- Idle gaps between kernel-active intervals: n=195786, p50 0.52 ms, p99 11.02 ms, max 2636.9 ms, total 137.81 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.5 | 14.6 | 89.1 | 0.70 | 3.06 |
| 2 | 5.8 | 15.5 | 42.8 | 0.71 | 2.85 |
| 3 | 6.1 | 15.6 | 27.4 | 0.72 | 2.79 |
| 4 | 4.8 | 15.2 | 28.9 | 0.70 | 2.86 |
| 5 | 5.2 | 15.9 | 33.0 | 0.69 | 2.74 |
| 6 | 6.4 | 16.2 | 24.9 | 0.72 | 2.67 |
| 7 | 6.0 | 16.1 | 19.0 | 0.74 | 2.68 |
| 8 | 5.5 | 16.2 | 42.0 | 0.70 | 2.72 |
| 9 | 6.2 | 16.6 | 30.2 | 0.70 | 2.60 |
| 10 | 6.5 | 0.0 | 49.1 | n/a | n/a |
