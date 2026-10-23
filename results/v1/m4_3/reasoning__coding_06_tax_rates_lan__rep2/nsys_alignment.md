# Nsight Systems alignment: baseline_32k__coding_06_tax_rates_lan__00

47014 kernels and 9161 CUDA graph launches captured (llama.cpp decodes with CUDA graphs); GPU-active time inside the run: 4.13 s.

- GPU-active time inside proxy LLM windows: **100.0 %** (acceptance >= 90 %)
- Decode-window 100 ms slices with GPU activity: **85.1 %**
- GPU-active share of LLM time: 9.8 % vs NVML utilisation 77.4 %
- Idle gaps between kernel-active intervals: n=50233, p50 0.00 ms, p99 19.36 ms, max 35.7 ms, total 35.68 s

| step | GPU-active % prefill | GPU-active % decode | NVML busy % | gap p50 ms | gap p99 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 8.7 | 12.7 | 77.4 | 0.73 | 3.62 |
