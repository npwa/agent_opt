# Nsight Systems alignment: baseline_32k__logtriage_04_pool_exhaustion__00

116896 kernels and 62532 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 16.75 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **96.1 %**
- GPU-active share of LLM time: 12.4 % vs NVML utilisation 58.2 %
- Idle gaps between kernel-active intervals: n=173486, p50 0.00 ms, p99 17.78 ms, max 356.1 ms, total 115.49 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.7 | 16.1 | 82.1 | 0.72 | 2.70 |
| 2 | 9.1 | 16.8 | 57.8 | 0.71 | 2.63 |
| 3 | 9.3 | 16.1 | 61.0 | 0.75 | 3.36 |
| 4 | 9.4 | 17.1 | 59.6 | 0.70 | 2.74 |
| 5 | 8.4 | 15.3 | 30.7 | 0.73 | 2.80 |
