# Direction for the Team: Next-Stage Recommendation

## Recommended Architecture and Operating Point

OpenCode + Ollama-served `qwen3.6`, with task-specific custom agents (`comms`, `research`)
rather than a single general-purpose agent, and MCP-based tool integration (SearXNG,
communication tools) over local stdio servers. Context window is tuned per task family, not
fixed globally: ~16k for coding/general tasks, up to 64k reserved for search-heavy research
tasks that accumulate large retrieved context.

**Rejected alternatives:**
- *Cloud-model fallback for weak spots* — explicitly out of scope; the assignment requires a
  local-inference path, and papering over local model gaps with a cloud call would hide the
  exact reliability data this project exists to produce.
- *One fixed context window for all tasks* — wastes VRAM on tasks that don't need it and, per
  our data, doesn't uniformly help quality even where affordable.
- *Steering agent behavior via user prompt phrasing* — fragile and doesn't scale; agent-level
  system prompts (config, not prompting discipline) proved more reliable and are the
  recommended pattern going forward.

## Standardize Now vs. Leave as Experiment

**Standardize:**
- Proxy + sampler instrumentation, paired with a standing requirement to cross-check any
  anomalous result against an uninstrumented baseline before attributing it to the model.
- Task-specific agent configs (`.opencode/agent/*.md`) as the mechanism for behavior tuning.
- Mock-by-default, two-factor opt-in for any tool with real-world side effects (mirrors the
  comm-tools design: safe by construction, not by which mode happens to be active).

**Leave as experiment:**
- Exact context-window size per task type — directionally useful, not yet validated across
  enough task categories to lock in as a default.
- Whether citation-provenance enforcement belongs in the model's generation constraints
  (current approach, partial success) vs. a post-hoc automated verification step (recommended
  next experiment, see Stage 2 below).
- Multi-model comparison — config now supports switching models per run, but a real head-to-head
  hasn't been completed at adequate sample size.

## Adapting Across B390 / B370

This project's hardware (RTX 3080, discrete 10GB VRAM) diverges from the original target
(Panther Lake, unified memory) — a deliberate, documented substitution. The core finding —
context-window size trades directly against how much of the model fits in fast memory —
should generalize conceptually to a unified-memory device, but the specific 16k/32k/64k
thresholds found here do not transfer directly and must be re-baselined on real silicon.
B390 and B370 differ in Xe core count, not just clock; don't assume B370 behavior can be
linearly derived from B390 results — re-run the context-window comparison independently on
each.

## Main Risks and Retirement Plan

- **Tool-selection unreliability** (model bypasses a provided tool for a familiar training-data
  pattern — observed independently at least three times this project). Retire via mandatory
  tool-naming in agent system prompts and narrower per-agent tool schemas; both showed partial
  success, neither fully closes the gap.
- **Citation/fact fabrication survives strict agent-level constraints** (best case so far: 2 of
  6 citations per research run still fabricated). Retire by moving verification out of the
  model and into an automated post-hoc check against retrieved tool output, rather than further
  prompt iteration — prompt-only mitigation has a demonstrated ceiling.
- **Instrumentation-induced false failures** (three confirmed cases this project: a
  Content-Length mismatch, an SSE framing bug, and a `stream_options` side effect — each looked
  like a model/harness failure and wasn't). Retire by keeping an uninstrumented fallback path
  available for cross-checking any surprising result.
- **Convenience-wrapper config drift** — `ollama launch` silently doesn't forward MCP or agent
  flags from the full config. Retire by standardizing on direct `opencode run`/`opencode`
  invocation for anything beyond quick manual checks.
- **Live-credential/consequential-action risk** — already retired for communication tools via
  mock-by-default plus explicit two-factor live opt-in; carry this pattern forward as a
  non-negotiable default for any future tool with external side effects.

## Staged Validation Plan

| Stage | Owner | Goal | Gate to proceed |
|---|---|---|---|
| 1 | Harness/infra | Re-baseline context-window thresholds on real Panther Lake hardware | Results within expected order of magnitude of RTX 3080 findings |
| 2 | Agent/prompt eng. | Build automated post-hoc citation verification against retrieved tool output | Measurable drop in unverifiable-citation rate on a larger held-out research set |
| 3 | Product | Extend communication tools to a second workflow pattern (e.g. multi-recipient email, calendar conflict handling) under the same mock-by-default design | Zero live-credential incidents; dedup false-negative rate stays at 0% on an expanded test matrix |
| 4 | Eval/benchmarking | Run a genuine head-to-head against a second model on the full task suite | Adequate sample size per task category (current n=1–2 on several tasks is too thin to trust) |

## What to Stop Doing If Measurements Invalidate This Direction

- If Stage 1 shows the context/VRAM tradeoff doesn't hold on unified memory (e.g. bandwidth,
  not capacity, is the real bottleneck there) — stop treating context-window tuning as the
  primary optimization lever and pivot to a bandwidth-focused investigation instead.
- If Stage 2 shows the model's real citation-accuracy ceiling can't be moved by any harness-level
  intervention — stop iterating on agent-prompt wording for this and either accept it as a
  documented model limitation requiring mandatory human review, or swap models for the research
  workload specifically.
- If tool-selection unreliability recurs despite narrowed schemas and explicit naming — stop
  treating it as a solvable prompt-engineering problem and document it as a hard reliability
  ceiling of this model/quantization for agentic tool use.

----

> **Note:** This direction document, along with portions of the accompanying README and
> results writeups, was drafted with AI assistance based on the author's own testing,
> debugging, and analysis throughout this project.
