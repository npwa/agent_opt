# Hung trials (kept for the record, excluded from the results)

`baseline_32k__c_ringbuf_03_ringbuffer__p00` (uninstrumented parity trial, started 2026-10-21 10:14 PDT):
OpenCode finished its first step, then waited about 18 minutes for a response from Ollama while Ollama and
the GPU were idle. The runner ended it at the task timeout (1140 s) and recorded it as a `timeout` outcome,
which it was not: it is a harness fault. The only such hang in about 230 instrumented and 56 parity trials.
It was re-run after the stall detector (`harness/stall.py`, plan item P13) was added; the re-run is the
trial in `../trials/`. Its record and events are kept here unchanged.
