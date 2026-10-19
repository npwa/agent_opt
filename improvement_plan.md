# agent_opt — Project Plan

**Working title:** *Characterizing and Evaluating a Local Coding/Research Agent Harness on a 10 GB GPU*
(upgrade of `agent_opt`, currently "OpenCode + Ollama proof of concept")

**Status of this document:** planning only, nothing implemented. Written 2026-10-11 against the
repo state at commit `358f22f` (read-only review of `~/work/agent_opt`).

**Target roles:** JR2024262 (AI Inference Performance), JR2022556 (Agentic AI, HP code-gen),
JR2023179 (Agent Architecture and Evaluation), JR2024664 (High Performance AI, CUDA agents).
Already applied: JR2021914 (this project also supports its interview questions on profiling,
CI/CD and local agentic deployment).

---

## Progress log

- **M0 done (2026-10-12):** tag `v0-first-pass`, `results/v0/`, package skeleton, CI, ported proxy/sampler/report with tests, `harness/manifest.py`.
  Deviations: `join_metrics.py` was never committed in v0, so `harness/join_metrics.py` is a reconstruction from the `report.py` column contract; no raw v0 data exists, so characterization tests use synthetic fixtures.

- **M1 step 1 done (2026-10-18):** task YAML schema + validator (`harness/taskspec.py`, `python -m harness.taskspec`), 8 existing tasks converted to `tasks/*.yaml` with fixtures in `fixtures/tasks/`, 21 negative-case tests. Verifier types so far: `rubric`, `compile_run`, `c_test`, `command`, `python`; hidden verifier assets go in `fixtures/verify/<id>/` (only `coding_03` has one so far); `harness.verifiers.comms` is referenced but not yet written (step 3).
  M0 adjustments: manifest now records the project-local llama.cpp commit (`LLAMA_CPP_DIR`, default `~/work/agent_opt_ad/llama.cpp`); `.gitignore` covers editor backup/lock files.
  Prompt changes vs v0: tasks 02–04 now name the output file and use relative paths (workdir is a temp copy of the fixture); task 03 writes `compress.c`.

---

## 1. Where the project stands today

What exists (verified by reading the repo):

| Area | Current state |
|---|---|
| Stack | OpenCode → Ollama (`qwen3.6`, 16k/32k `num_ctx`), custom agents `comms` and `research`, MCP servers (SearXNG, `comm_tools_mcp.py`) |
| Instrumentation | `benchmark/metrics_proxy.py` (174 lines, logging proxy in front of Ollama), `sampler.py` (1 Hz CPU/GPU/RAM via polling), `join_metrics.py`/`report.py` (per-task averages) |
| Tasks | 4 coding, 2 reasoning, 1 comms workflow, 1 deep-research prompt (n = 1–2 runs for most) |
| Findings | halving `num_ctx` 32k→16k: TTFT/total time -6…-26 % on 4 of 6 tasks, GPU util up, but 2 of 4 coding tasks regressed in correctness/verification honesty; `comms` agent cut model calls 36 → 12 (-67 %, ≈ -63 % wall-clock); research agent still fabricated 2 of 6 citations; 3 instrumentation-induced false failures documented |
| Gaps (stated by the author in `DIRECTION.md`) | n = 1–2 per task is "too thin to trust"; citation verification is prompt-only; no second-model comparison; no uninstrumented cross-check automation; Panther Lake/Arc not available |

What it is **not** yet: a statistically defensible evaluation, a view of what the *GPU* does
while an agent works, or a test suite. Those three gaps are exactly what the four target roles
ask for.

## 2. Why this project, why now (fit to the four roles)

| Role requirement (from the posting) | What this project will demonstrate |
|---|---|
| JR2023179: "architecting systems that analyze inference workload dynamics of leading agentic harnesses"; "evaluate performance of AI agents"; "rigorous analytical approach to data collection" | Per-step agent trace → prefill/decode/tool/idle breakdown, context growth, prompt-cache reuse; bootstrap confidence intervals; n ≥ 10 runs per task |
| JR2022556: "design benchmarks for complex, non-deterministic agentic behaviors"; coding-agent skills/workflows (Claude Code, Cursor, Codex); Python + C++/Rust; high-throughput orchestration | Executable-verifier task suite with pass@k and flake rate; agent configs as code; comparison of agent configurations; CI. The harness is Python only; C/C++ appears in the coding tasks and in the llama.cpp/Nsight side (see `RTX_3080_opt`) |
| JR2024664: "reproducible, high-fidelity evaluation frameworks covering performance, quality and developer productivity"; optimize on "resource-constrained platforms" | One framework reporting latency, GPU duty cycle, task success and tool-call efficiency on a 10 GB card |
| JR2024262: TTFT / ITL / P99, KV-cache management, speculative decoding, batching, Nsight Systems, "AI-agent-supported performance workflows" | Agent workloads as the *measured* object: TTFT/ITL percentiles per call, KV/`num_ctx` capacity study, speculative decoding with a same-family draft, Nsight Systems timeline of an agent session |

Should `agent_opt` be added to the resume's "Recent Independent Projects"? **Yes, but only after
Milestones M1–M4** (§6). As it is today it reads as notes from a first pass; after the plan it is
the most direct evidence for three of the four target roles (it is the only project that
measures an agent harness as an inference workload).

## 3. Goals and non-goals

### Goals
G1. Turn the benchmark into a **statistically defensible evaluation** (n ≥ 10 per task, CIs, pass/fail verifiers).
G2. Explain **where time goes inside an agent run** (LLM prefill, decode, tool execution, harness overhead, GPU idle) and which knob fixes which part.
G3. Replace prompt-only citation hygiene with an **automated post-hoc citation verifier** (DIRECTION.md Stage 2).
G4. Test three **inference-side levers** specific to agent loops: prompt/prefix cache reuse + prompt trimming, KV-cache/`num_ctx` sizing, speculative decoding with a same-family draft model.
G5. Make the harness **self-testing** (unit tests, replay mode, fault-injection, CI) so results are trustworthy.
G6. Produce a **10-slide deck** and a README results dashboard.

### Non-goals
- No cloud-model fallback (keeps the original scope in `DIRECTION.md`).
- No Panther Lake / Arc validation (hardware not available); the plan keeps the platform
  abstraction (`platform.yaml`) so a future run is a config change.
- No model training/fine-tuning.
- Not a general agent benchmark (e.g. SWE-bench). The suite stays small and fully executable locally.

## 4. Requirements

IDs are used by the test plan (§7).

### 4.1 Functional requirements

| ID | Requirement | Priority |
|---|---|---|
| F1 | A **task spec format** (YAML) per task: prompt(s), working-dir fixture, tool permissions, agent/config, verifier, timeout, expected tool sequence (optional) | Must |
| F2 | **Suite ≥ 14 tasks**: the existing 8 plus ≥ 6 new ones: bug-fix with unit tests, multi-file refactor, C compile-and-run, log-triage with tool use, repo Q&A (grounded), long-context file summarization (stresses `num_ctx`) | Must |
| F3 | **Executable verifiers** for each coding task (compile, run, diff vs expected output, unit tests); for text tasks: rubric checks that are programmatic (length limits, required facts, forbidden claims) | Must |
| F4 | **Trial runner**: runs each (task × configuration) N times (default 10) with fixed seeds where the backend supports it, randomised order (to spread thermal/cache drift), resumable, writes one JSON record per trial | Must |
| F5 | **Trace schema** joining proxy records, tool-call events (from OpenCode session export / MCP logs) and GPU samples into a per-step timeline: `{step, llm_call_id, t_start, ttft, prefill_tokens, decode_tokens, itl_p50/p99, tool_name, tool_latency, gpu_busy_pct, idle_gap}` | Must |
| F6 | **Time decomposition** per run: Σ prefill, Σ decode, Σ tool execution, Σ harness/idle, as percentage of wall-clock; GPU duty cycle | Must |
| F7 | **Direct-vs-proxy parity check** automated: each suite run includes N uninstrumented runs of a subset; report flags a parity violation if success rate or total time differs > 5 % (CI-adjusted) | Must |
| F8 | **GPU sampler upgrade**: NVML (`pynvml`) at ≥ 10 Hz for utilization, memory, power, SM/mem clocks, PCIe throughput; CPU/RAM stays at 1 Hz | Must |
| F9 | **Nsight Systems capture mode**: wrap the Ollama/`llama-server` process for one representative run per config; extract kernel-active vs idle time and CPU-side launch gaps (`nsys stats` → CSV) and align with proxy timestamps | Should |
| F10 | **Citation verifier**: parse citations from the final answer, match each against retrieved tool output of that run (exact author/title/year/venue fields), optionally check DOIs offline against a cached Crossref lookup; classify each citation as `confirmed`, `minor-error`, `fabricated`, `unsupported`; compute fabricated-citation rate | Must |
| F11 | **Config sweep**: factor levels `num_ctx ∈ {8k,16k,32k,64k}`, KV-cache type `{f16,q8_0}`, flash-attn `{on,off}`, agent config `{default, task-specific}`, model `{qwen3.6, one second model}`; fractional-factorial or one-factor-at-a-time with a documented design | Must |
| F12 | **Speculative decoding experiment**: `llama-server` with a same-family draft model (vocab compatibility verified first, lesson from `TensorRT_plugin` Phase 6: shared tokenizer ≠ agreement); report acceptance rate per task category (tool-call JSON vs prose vs code) | Should |
| F13 | **Prompt-budget analysis**: tokens spent on system prompt + tool schemas vs conversation per call (baseline already shows ≈ 33 K input tokens per 2–3 calls in `inf_opt` Phase 0); try schema trimming and agent-prompt minimisation; report prefill saved vs success rate change | Must |
| F14 | **Report generator**: Markdown + HTML (static) with tables, CI error bars, time-decomposition stacked bars, per-config Pareto (success vs total time) | Must |
| F15 | **Replay mode**: run the whole pipeline (join, stats, report, verifier) from recorded traces without a GPU/model | Must |
| F16 | **Mock-by-default safety** for side-effect tools retained (two-factor opt-in for live email/calendar) | Must (existing) |

### 4.2 Non-functional requirements

| ID | Requirement |
|---|---|
| N1 | Full suite on one configuration finishes in ≤ 3 h on the RTX 3080 box; full sweep is resumable and runs unattended overnight |
| N2 | All raw data (JSONL per trial) committed or reproducibly regenerable; summary tables regenerate from raw data with one command |
| N3 | Every reported delta states n, median, 95 % CI, and the test used; no claims from n < 5 |
| N4 | Python 3.12, type hints on new modules, `ruff` clean, `pytest` ≥ 85 % line coverage on the analysis/verifier code (not the model-dependent runner) |
| N5 | CI (GitHub Actions) runs unit + replay tests on every push without GPU |
| N6 | Methodology section documents each known instrumentation failure mode and the test that prevents its recurrence |

### 4.3 Success criteria (project-level)

1. Suite of ≥ 14 executable tasks × ≥ 10 trials × ≥ 6 configurations, reported with CIs.
2. At least **two** levers shown to move a target metric with a CI that excludes zero, **and** at least one lever reported as a negative result with its causal chain (the repo's established style).
3. Fabricated-citation rate measured on ≥ 20 research prompts (was: 2 of 6 in one run), with and without the verifier gate.
4. Time-decomposition figure that answers "what fraction of agent wall-clock is GPU decode?" for each task category.
5. Parity check shows proxy overhead < 5 %.
6. Deck + README updated; resume bullet drafted with measured numbers.

## 5. Design

```
 tasks/*.yaml ──► runner ──► OpenCode ──► metrics_proxy ──► Ollama / llama-server
                    │            │             │ (JSONL)            │
                    │            └ session export (tool events)     └ NVML sampler (10 Hz)
                    ▼                                               └ optional nsys capture
              trial JSONL ──► join (trace schema F5) ──► stats (bootstrap, tests)
                                                    ├─► citation verifier (F10)
                                                    └─► report/HTML (F14)
 replay: trial JSONL + fixtures replace everything left of "trial JSONL"
```

Key design choices:
- **Proxy stays a passive observer**: add a `--passthrough-bytes` mode that forwards raw bytes
  untouched and only timestamps chunks (removes the `stream_options` class of bug). Token
  counts come from Ollama's own final `eval_count`/`prompt_eval_count` fields, not request edits.
- **One event timeline per run** keyed by monotonic clock; wall-clock only for display.
- **Statistics:** percentile bootstrap (10 000 resamples) for medians and ratios; Mann–Whitney
  U for two-config comparison; Holm correction when several levers are compared; effect sizes
  reported alongside p-values. Success rate: Wilson interval.
- **Task specs and agent configs live in git** so every result maps to a commit.

## 6. Implementation plan

Estimates assume ~10–12 h/week; total ≈ 4–5 weeks. Each milestone ends with a commit tagged
`m1`…`m6` and a short results note (so partial progress is already presentable).

### M0 — Baseline freeze and scaffolding (2–3 days)
1. Tag current state `v0-first-pass`; copy existing reports to `results/v0/` (no deletion).
2. Create `tasks/`, `harness/` (package), `analysis/`, `tests/`, `fixtures/` directories; add `pyproject.toml`, `ruff`, `pytest`, `.github/workflows/ci.yml`.
3. Port `metrics_proxy.py`, `sampler.py`, `report.py` into the package with behaviour-preserving tests first (characterization tests against recorded v0 data).
4. Record hardware/software manifest (`platform.yaml`: GPU, driver, CUDA, Ollama, OpenCode, model digests) automatically at run start.

### M1 — Task suite with executable verifiers (4–5 days)
1. Define task YAML schema (F1) and validator; convert the 8 existing tasks.
2. Write ≥ 6 new tasks (F2) with fixtures: e.g. `bugfix_01` (failing pytest → fix), `refactor_02` (rename + update call sites, tests must pass), `c_ringbuf_03` (compile with `-Wall -Werror`, run test vector), `logtriage_04` (find root cause in a 5 MB log using grep tools), `repoqa_05` (grounded answer requiring file reads; verifier checks cited line numbers), `longctx_06` (summarise a ~12 k-token file; verifier checks N facts).
3. Implement verifiers (F3) as small Python callables run in a throw-away container/temp dir (no network, resource limits).
4. Smoke-run each task once to calibrate timeouts and remove ambiguity (record the calibration run, don't hide it).

### M2 — Trial runner, trace join, GPU telemetry (5–6 days)
1. Runner with randomised order, retries only for harness failures (never for wrong answers), resumable state file (F4).
2. Capture OpenCode session events through `opencode run --format json` (or session export) and MCP-side logs; define the trace schema (F5).
3. NVML sampler at 10 Hz (F8); correlate to proxy chunk timestamps; compute GPU busy %, idle gaps during tool calls.
4. Time-decomposition computation (F6) with an explicit "unattributed" bucket that must be < 5 % (otherwise the instrumentation is wrong).
5. Automated parity check (F7) built into the runner (`--parity-fraction 0.2`).
6. Optional `nsys` mode (F9): wrap the server, capture a single run, export `cuda_gpu_kern_sum` and `cuda_api_sum`; add a figure of kernel-active vs idle over the agent timeline.

### M3 — Statistics, report, replay (3 days)
1. Bootstrap/Wilson/Mann–Whitney/Holm utilities with unit tests against known distributions (N3).
2. Report generator (F14); stacked time-decomposition bars, success-vs-time Pareto, per-task tables.
3. Replay mode (F15) and CI job that runs the whole analysis on committed fixture traces.

### M4 — Baseline characterization campaign (3–4 days, mostly unattended)
1. Run suite × default config × n = 10 → `results/v1/baseline/`.
2. Run the 32k vs 16k comparison again with n = 10; re-check the earlier claims (16k regression on fizzbuzz-style tasks, verification-honesty). Report whether the v0 conclusions survive.
3. Publish the time-decomposition result for each task category.

### M5 — Lever experiments (7–8 days)
1. **Context/KV sizing** (F11): `num_ctx` × KV type × flash-attn; record VRAM, layers offloaded to CPU (Ollama log), decode tok/s, success.
2. **Prompt-budget** (F13): measure tokens by prompt component per call; variants: (a) default, (b) trimmed tool schemas (only the tools a task needs), (c) shortened agent system prompt; confirm prefix-cache reuse via `prompt_eval_count` on consecutive calls; hypothesis: prefill is a large share of wall-clock in multi-step tasks, and trimming reduces it without lowering success.
3. **Speculative decoding** (F12): pick a draft model of the same family and verify tokenizer identity before any timing; run `llama-server -md <draft>` against the same task traces (replayed prompts) so acceptance rate is measured on real agent output; split by output type (tool-call JSON, code, prose). Prediction to test: higher acceptance on structured tool calls than on prose; overall gain uncertain given the 10 GB limit (draft consumes VRAM). Report a negative result honestly if it occurs.
4. **Second model** (F11): one comparison model at similar VRAM footprint; n = 10; same suite.
5. **Agent config** (existing finding): re-verify `comms` agent call reduction (36 → 12) with n = 10 and add `coding` agent variant that mandates a verification step (targets the "claims tests it didn't run" regression); measure effect on false-verification claims via transcript check (did a `bash` tool call precede each "tests pass" claim?).

### M6 — Citation verifier and research evaluation (4–5 days)
1. Implement the verifier (F10): extract citations with a conservative regex + structured-output fallback; match against the run's retrieved pages (stored by the MCP bridge); produce a labelled table like the existing free-will audit.
2. Build a **gold set**: the existing audited run (6 citations with known verdicts) plus ≥ 14 more research prompts audited manually once (keep the label file in git).
3. Measure verifier precision/recall against the gold set; target ≥ 90 % agreement.
4. Evaluate: research agent without gate vs with **post-hoc gate** (drop/mark unverified citations and force a rewrite pass); measure fabricated-rate and answer usefulness (fact-coverage score).

### M7 — Write-up (3–4 days)
Update README (lead with the result, not the setup; follows the lesson from `Evol_inference/project_review.md`), `DIRECTION.md` (Stage 2 now done, Stage 4 partially), AI-disclosure, slide deck, resume bullet.

## 7. Test plan

### 7.1 Unit tests (CI, no GPU)

| Test | Requirement | Pass condition |
|---|---|---|
| Task YAML validator rejects malformed specs (missing verifier, bad timeout) | F1 | 12 negative cases rejected with clear messages |
| Each verifier returns correct pass/fail on a golden-good and golden-bad artifact | F3 | all tasks: good → pass, bad → fail |
| Trace join on fixture proxy+tool+GPU logs reproduces a hand-built expected timeline | F5 | exact match on all fields |
| Time decomposition on synthetic timelines (known sums) | F6 | unattributed < 1 % on synthetic; sums to wall-clock |
| Bootstrap CI coverage on synthetic data (1 000 simulated experiments) | N3 | nominal 95 % interval covers true value 93–97 % |
| Wilson interval, Mann–Whitney, Holm against `scipy` reference values | N3 | agree to 1e-9 |
| Citation verifier on the labelled six-citation table | F10 | reproduces all six verdicts (3 confirmed/minor, 3 fabricated/unsupported as audited) |
| Citation verifier on gold set | F10 | ≥ 90 % verdict agreement, 0 false "confirmed" on fabricated entries |
| Replay mode regenerates a committed report byte-for-byte | F15 | hash match |
| Safety: mock tools refuse live mode without both opt-ins; duplicate-detection normalises attendee order/whitespace | F16 | regression test for the earlier hash bug |

### 7.2 Fault-injection tests for the proxy (regression for the three documented failure modes)

| Injected fault | Expected behaviour |
|---|---|
| Request body modified without fixing `Content-Length` | proxy detects/avoids; test fails if mismatch can occur |
| SSE stream missing the blank-line delimiter | client still receives a parseable stream; test compares byte stream to upstream |
| `stream_options` added to request | proxy must not alter request in passthrough mode; byte-equal request at upstream mock |

### 7.3 Integration / system tests (GPU box)

| Test | Pass condition |
|---|---|
| End-to-end smoke: 1 task × 2 trials with a tiny model | JSONL, report and parity check produced; unattributed time < 5 % |
| Parity (F7): 20 % of trials uninstrumented | success-rate difference within Wilson CI overlap; total-time difference < 5 % |
| Determinism: same seed/config twice at temperature 0 | identical token counts on ≥ 90 % of calls (otherwise report the nondeterminism level and widen N) |
| GPU sampler cross-check vs `nvidia-smi dmon` | utilization within ±5 pp over a 60 s steady load |
| Nsight Systems alignment | proxy-reported decode windows overlap with kernel-active windows ≥ 90 % |
| Speculative decoding output equivalence | greedy outputs with and without draft are token-identical (as in the `TensorRT_plugin` check) before any timing is trusted |

### 7.4 Experiment acceptance rules
- A lever is "positive" only if the 95 % CI of the improvement excludes 0 **and** success rate does not drop by more than 5 pp (Wilson, per task category).
- Every result table lists n; tables with n < 5 are marked "directional".
- Any surprising result must first be re-run with the uninstrumented path (rule already in `DIRECTION.md`).

### 7.5 Risks and mitigations

| Risk | Likelihood | Mitigation |
|---|---|---|
| Non-determinism makes n = 10 insufficient | medium | measure variance first (M4); increase n only on high-variance cells; report CIs |
| Draft model for speculative decoding is not same family/vocab | medium | verify tokenizers first; if no valid draft exists, report as blocked and substitute prompt-lookup/n-gram speculation in `llama-server` |
| Sweep too large for the time budget | medium | fractional design, cut factors in order: second model, flash-attn, 8k |
| OpenCode/Ollama version drift mid-project | low | pin versions in `platform.yaml`, record digests |
| Time decomposition has large unattributed bucket | medium | add finer events; if still > 5 %, state limitation |
| Task verifiers too lenient/strict | medium | calibration runs in M1, golden-good/bad tests |

## 8. Deliverables

1. Upgraded repo (`harness/`, `tasks/`, `analysis/`, `tests/`, CI badge).
2. `results/v1/` raw JSONL + `RESULTS.md` dashboard.
3. Updated `README.md`, `DIRECTION.md`, AI-disclosure.
4. Slide deck `presentation/agent_opt.pptx` (concept below).
5. Resume entry (draft below, numbers to be filled in).

### Draft resume entry (fill after M5/M6)
> **agent_opt:** Evaluation and inference-characterization framework for a local coding/research agent (OpenCode + Ollama, RTX 3080 10 GB): 14 executable tasks × 10 trials × 6 configurations with bootstrap CIs; decomposed agent wall-clock into prefill / decode / tool / idle using NVML and Nsight Systems; cut model calls 67 % and prefill by **X %** via task-specific agents and prompt-budget trimming; automated citation verifier reduced fabricated citations from **A %** to **B %**; speculative decoding acceptance **C %** (**result**).

## 9. Slide deck concept (≈ 10 main slides + 3 backup)

Audience: hiring manager/engineers (technical, 15–20 min). Style: one message per slide, result in the title, data on the slide, setup in backup.

1. **Title + one-line thesis** — "Agent loops are inference workloads: where does the time go on a 10 GB GPU?" Hardware strip (RTX 3080, 10 GB, i7-11700K).
2. **Why it matters** — three-box diagram: user task → agent loop (LLM calls + tools) → GPU; "same GPU is idle while tools run" teaser stat from M4.
3. **Method in one picture** — architecture diagram from §5 (runner → OpenCode → proxy → Ollama; NVML + Nsight side channels; verifiers; stats). Callout: "passive-observer proxy, uninstrumented parity check".
4. **Task suite and verifiers** — table of 14 tasks by category with verifier type; note on executable ground truth.
5. **Where the time goes** — stacked horizontal bars per task category: prefill / decode / tool / harness / idle (headline figure); annotation for the biggest bucket.
6. **Context window is a memory-placement knob, not a speed knob** — plot: `num_ctx` vs layers on GPU vs TTFT vs success rate with CI error bars; revisits v0 claim (16k faster, but quality effect?).
7. **Prompt budget** — waterfall of tokens per call (system, tool schemas, history, new input); before/after trimming; prefill saved vs success unchanged.
8. **Speculative decoding on agent output** — acceptance rate by output type (tool JSON / code / prose), speed-up (or slow-down) with CI; link to the earlier 9 % acceptance negative result in `TensorRT_plugin` and why same-family draft changes it.
9. **Making the agent honest** — citation verifier: confusion matrix on the gold set; fabricated-rate before/after gate (bar with CI); example of a caught fabricated citation.
10. **Lessons and what I'd do on NVIDIA RTX/DGX** — 3 bullets: measure with parity checks; fix at the harness not the prompt; port the framework to TensorRT-RTX/vLLM backends and DGX Spark; call to action.
- Backup A: instrumentation failure modes + regression tests (Content-Length, SSE, `stream_options`).
- Backup B: statistics (bootstrap, Holm), full result tables.
- Backup C: reproduction commands and platform manifest.

Visual rules: titles state the finding; error bars on every comparison; at most 1 figure + 3 bullets per slide; speaker notes carry the numbers' provenance (run IDs).

## 10. Open questions for the author (decide before M1)

1. Second model for the head-to-head: which model fits 10 GB at a comparable quant (same family preferred for the draft model too)?
2. Is it acceptable to include one run set using a cloud model as a *reference only* (never as a fallback)? Default in this plan: **no**, to keep the original scope.
3. Should the suite be renamed/published as a reusable benchmark (`local-agent-bench`), or stay inside `agent_opt`? Default: stay inside, extract later.
