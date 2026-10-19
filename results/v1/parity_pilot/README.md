# M2.5 parity pilot (proxy vs direct)

Command: `python -m harness.runner run --configs configs/baseline_32k.yaml --tasks coding_05_scientific_lan coding_06_tax_rates_lan coding_01_my_sqrt_review --n 5 --seed 21 --parity-fraction 1.0`

15 instrumented trials (through `harness/proxy.py`, passthrough mode) and 15 uninstrumented trials (OpenCode straight to Ollama), interleaved in random order, 65 minutes in total. Only trial records are kept here (events, artifacts and GPU samples are not).

Result (`parity.md`): **inconclusive**. Success rate identical (60 % vs 60 %). Time ratio direct/instrumented = 0.972, 95 % bootstrap CI 0.882-1.073: the point estimate shows no proxy overhead, but 15 pairs on tasks this noisy cannot show it is below 5 %. The check needs roughly 4x more parity trials (or lower-variance tasks) to reach "ok"; with the planned 14 tasks x 10 trials x 0.2 fraction it will have 28 parity trials.

Side findings from the same data (n = 5 per cell and path): `coding_06` fails the 150-word limit in 6 of 10 runs; `coding_01` fails in 6 of 10 runs, almost always because the review does not mention the possible infinite loop - the model-quality signal the task is meant to measure, but it confirms that n = 1 calibration hid a lot of variance (the single calibration runs passed).
