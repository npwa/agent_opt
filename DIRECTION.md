# Direction for the Team: Next-Stage Recommendation

## Recommended Architecture and Operating Point

OpenCode + Ollama-served `qwen3.6` with the **default agent** and MCP-based tool integration
(SearXNG, communication tools) over local stdio servers. The v1 measurements (see
`README.md`, "Headline results") changed two parts of the original recommendation: the custom
`comms` agent gave no measurable benefit when re-tested at n = 10 (the earlier call reduction
did not reproduce) and a verification-step agent did not raise the pass rate, so the default
agent stays; `research` remains a separate agent because research needs its own tools and a 64k
window. The context window is still tuned per task family: 32k as the general default, 16k is
acceptable for tasks whose working context stays under about 14k tokens (no overall loss
measured, but long-context tasks fail), 64k for search-heavy research (at 32k most answers
overflow). Speculative decoding stays off and the temperature stays at the default; the 8B
model is not a substitute.

**Rejected alternatives:**
- *Cloud-model fallback for weak spots* - explicitly out of scope; this project is scoped to a
  local-inference path, and papering over local model gaps with a cloud call would hide the
  exact reliability data this project exists to produce.
- *One fixed context window for all tasks* - wastes VRAM on tasks that don't need it and, per
  our data, doesn't uniformly help quality even where affordable.
- *Steering agent behavior via user prompt phrasing* - fragile and doesn't scale; agent-level
  system prompts (config, not prompting discipline) proved more reliable and are the
  recommended pattern going forward.

## Standardize Now vs. Leave as Experiment

**Standardize:**
- Proxy + sampler instrumentation, paired with a standing requirement to cross-check any
  anomalous result against an uninstrumented baseline before attributing it to the model.
- Environment hygiene for the prompt: any change to paths or environment variables that
  OpenCode prints into its prompt can break the prefix cache or leak private setup (both
  happened); check a prompt diff after each such change. Trials with a custom agent start
  with a cold cache (the working directory sits early in their short prompt).
- Post-hoc citation verification and the drop gate (`harness/citations.py`, `harness/citegate.py`).
- Mock-by-default, two-factor opt-in for any tool with real-world side effects (mirrors the
  comm-tools design: safe by construction, not by which mode happens to be active).

**Leave as experiment:**
- Exact context-window size per task type - measured on 18 tasks (16k vs 32k) and on the
  research prompts (32k vs 64k), but not on other hardware.
- Trimming unused tool schemas: cold first call is 26-34 % faster in replay, but the effect on
  success has not been measured.
- Whether citation-provenance enforcement belongs in the model's generation constraints
  (current approach, partial success) vs. a post-hoc automated verification step (recommended
  next experiment, see Stage 2 below).
- Multi-model comparison - one head-to-head exists (`qwen3:8b`, core suite, n = 3: 24 % vs
  66 % pass rate); a larger or different second model has not been tried.

## Adapting Across B390 / B370

This project's hardware (RTX 3080, discrete 10GB VRAM) diverges from the original target
(Panther Lake, unified memory) - a deliberate, documented substitution. The core finding -
context-window size trades against how much of the model fits in fast memory, and with about
two thirds of the expert weights in system memory the GPU is busy only 12 % of the time - should
generalize conceptually to a unified-memory device, but the specific 16k/32k/64k
thresholds found here do not transfer directly and must be re-baselined on real silicon.
B390 and B370 differ in Xe core count, not just clock; don't assume B370 behavior can be
linearly derived from B390 results - re-run the context-window comparison independently on
each.

## Main Risks and Retirement Plan

- **Tool-selection unreliability** (model bypasses a provided tool for a familiar training-data
  pattern - observed independently at least three times in v0). Not re-measured in v1; the
  v1 evidence is that agent-prompt changes (short prompt, verification step, `comms` agent) did
  not move the pass rate once confirmed at n = 10, so treat prompt wording as a weak lever.
- **Citation/fact fabrication survives strict agent-level constraints.** Verification now sits
  outside the model: on 27 answers to 20 prompts 28 % of 567 citations are not backed by the
  retrieved pages, and dropping them keeps 263 of 264 supported ones and 99 % of the text.
  Residual risk: "not backed" is not "invented" (the manual audit is pending), and the check
  cannot judge claims without a citation.
- **Over-claiming after weak self-checks** (5 of 60 code-task trials end with a false claim; the
  verification-step agent reduced it only suggestively, 2 vs 7, and added timeouts). Retire
  with a verifier outside the agent, not more prompt text.
- **Instrumentation-induced false failures** (three confirmed cases this project: a
  Content-Length mismatch, an SSE framing bug, and a `stream_options` side effect - each looked
  like a model/harness failure and wasn't). Retire by keeping an uninstrumented fallback path
  available for cross-checking any surprising result.
- **Convenience-wrapper config drift** - `ollama launch` silently doesn't forward MCP or agent
  flags from the full config. Retire by standardizing on direct `opencode run`/`opencode`
  invocation for anything beyond quick manual checks.
- **Live-credential/consequential-action risk** - already retired for communication tools via
  mock-by-default plus explicit two-factor live opt-in; carry this pattern forward as a
  non-negotiable default for any future tool with external side effects.

## Staged Validation Plan

| Stage | Owner             | Goal                                                                                                                                                   | Gate to proceed                                                                                  |
|-------|-------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------|
| 1     | Harness/infra     | Re-baseline context-window thresholds on real Panther Lake hardware                                                                                    | Results within expected order of magnitude of RTX 3080 findings                                  |
| 2     | Agent/prompt eng. | Build automated post-hoc citation verification against retrieved tool output (**built; unverifiable-citation rate 28 % -> 0 % on 20 research prompts; manual precision audit pending**) | Measurable drop in unverifiable-citation rate on a larger held-out research set (met on the 20 prompts) |
| 3     | Product           | Extend communication tools to a second workflow pattern (e.g. multi-recipient email, calendar conflict handling) under the same mock-by-default design | Zero live-credential incidents; dedup false-negative rate stays at 0% on an expanded test matrix |
| 4     | Eval/benchmarking | Run a genuine head-to-head against a second model on the full task suite (**partly done**: `qwen3:8b` on the core suite at n = 3; the full suite and a second candidate remain) | Adequate sample size per task category (n = 10 on the baseline; the second model has n = 3)      |

## What to Stop Doing If Measurements Invalidate This Direction

- If Stage 1 shows the context/VRAM tradeoff doesn't hold on unified memory (e.g. bandwidth,
  not capacity, is the real bottleneck there) - stop treating context-window tuning as the
  primary optimization lever and pivot to a bandwidth-focused investigation instead.
- If Stage 2 shows the model's real citation-accuracy ceiling can't be moved by any harness-level
  intervention - stop iterating on agent-prompt wording for this and either accept it as a
  documented model limitation requiring mandatory human review, or swap models for the research
  workload specifically.
- If tool-selection unreliability recurs despite narrowed schemas and explicit naming - stop
  treating it as a solvable prompt-engineering problem and document it as a hard reliability
  ceiling of this model/quantization for agentic tool use. (v1 already points this way: three
  agent-prompt interventions gave no confirmed gain.)

----

> **Note:** This direction document, along with portions of the accompanying README and
> results writeups, was drafted with AI assistance based on the author's own testing,
> debugging, and analysis throughout this project.
