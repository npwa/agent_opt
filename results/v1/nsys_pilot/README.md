# M2.6 Nsight Systems pilot

One trial (`bugfix_01_moving_average`, baseline 32k) captured with `python -m harness.nsys capture`:
a private `ollama serve` (127.0.0.1:11436) wrapped in `nsys profile`, the metrics proxy in front of
it, the normal runner and NVML sampler on top. Raw `.nsys-rep` / sqlite are not kept (hundreds of MB
of tracer data); this directory has the aligned summary: `nsys_alignment.md/json`, `kernel_summary.csv`.

Acceptance (plan 7.3, "decode windows overlap kernel-active windows >= 90 %"): **met** - 100 % of
GPU-active time lies inside proxy LLM windows, and 91.8 % of 100 ms slices in decode windows contain GPU activity.

What it shows
* **Real GPU-active time is about 10 % of LLM time (15 % of decode, 5-8 % of prefill) while NVML
  reports ~50 % utilisation.** NVML marks a sample window busy if any kernel ran in it. With about two thirds of this
  model on the CPU, the GPU does short bursts (median graph 165 us) separated by CPU work. The CPU, not the GPU,
  is the bottleneck of this configuration - which is also why prefill runs at only ~250 tok/s.
* Decode runs as CUDA graphs (33,490 graph launches vs 87,256 individual kernels). They are stored in a separate nsys table; reading only
  the kernel table shows an almost idle GPU in decode (this was the first, wrong, result).
* GPU time by kernel: CUDA graphs 55 %, `mul_mat_q` 28 % (prefill matmuls), `gated_delta_net_cuda` 6 %,
  `flash_attn_ext_f16` 2 % (`kernel_summary.csv`).
* Idle gaps between GPU activity: median 0.7-0.8 ms in decode, p99 ~3 ms - the CPU-side launch/compute latency per token.

Method notes
* Ollama 0.32 serves through a `llama-server` child process. Required nsys flags: `--trace-fork-before-exec=true` to follow it and
  `--cuda-flush-interval=500`: Ollama kills the child on unload and without periodic flushes **all its kernel records are lost**
  (first two captures showed 0 kernels).
* The system Ollama service runs as user `ollama` and cannot be wrapped without root, so a second instance is started on another port with
  the same read-only model store; the service must have no model loaded. The capture checks this and aborts otherwise.
* CUDA hardware tracing is unsupported on this GPU/driver; nsys uses a software-instrumented trace, so absolute kernel durations carry
  some overhead. The trial itself passed (104 s, similar to uninstrumented runs).
