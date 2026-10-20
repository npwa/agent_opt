# Replay fixtures (F15)

Two **real** recorded runs (RTX 3080, OpenCode 1.18.11, Ollama 0.32.5) that the whole analysis can be
re-derived from without a GPU or model: `python -m harness.replay --run fixtures/replay/<name> --out <dir>`.

| Fixture | What it is | Exercises |
|---|---|---|
| `parity_pilot/` | the 30 trial records of the M2.5 parity pilot (15 instrumented, 15 direct), records only | verifier replay (30 verdicts re-verified from recorded final answers), parity check, statistics, report without traces |
| `traced_pilot/` | 2 complete trials (events, artifacts, proxy records, 10 Hz NVML samples) | trace join, time decomposition, GPU numbers, verifier replay from artifacts (`bugfix_01` compiled/tested from the recorded `stats_utils.py`) |

`expected/` holds the committed golden output of each replay (`RESULTS.md`, `report.html`, `summary.json`,
`parity.*`, `replay_verdicts.json`, `SHA256SUMS`). `tests/test_replay.py` and the `replay` CI job regenerate them
and require byte-for-byte equality. After an intentional change to the analysis, refresh the goldens with

    python -m harness.replay --run fixtures/replay/<name> --out fixtures/replay/<name>/expected
