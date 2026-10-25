# Agent_opt: characterizing a local coding/research agent on a 10 GB GPU

**Question.** Where does the time go when a local AI coding agent (OpenCode driving an Ollama model) works on a
10 GB graphics card, and which changes make it faster or better without making it worse? Everything runs locally
(RTX 3080, 10 GB; no cloud models), and every claim is meant to come with a sample size, a confidence interval
and a way to re-derive it from the recorded runs.

For a non-technical overview see the slide deck [`Doc/Agent_opt.pptx`](Doc/Agent_opt.pptx). The full plan,
requirements and progress log are in [`improvement_plan.md`](improvement_plan.md); the original direction and its
limits are in [`DIRECTION.md`](DIRECTION.md).

## Status

| Milestone | What | State |
|---|---|---|
| M0 | Baseline freeze, package scaffolding, CI, platform manifest (tag `m0`) | done |
| M1 | 14 executable tasks with verifiers, sandbox, golden tests, calibration run (tag `m1`) | done |
| M2 | Trial runner, trace join, 10 Hz GPU telemetry, time decomposition, parity check, Nsight capture (tag `m2`) | done |
| M3 | Statistics, report generator, replay mode with golden fixtures and a CI job (tag `m3`) | done |
| M4 | Baseline campaign: whole suite, n = 10, 32k vs 16k re-check | **done** (tag `m4`): M4.1 done (two 168-trial baselines: first and isolated); M4.2 done (16k vs 32k); M4.3 done (GPU activity); tag `m4` |
| M5 | Lever experiments: context/KV size, prompt budget, speculative decoding, second model, agent configs | planned |
| M6 | Automated citation verifier and research evaluation | planned |
| M7 | Write-up (README, deck, results) | planned |

Earlier pilots: `results/v1/calibration/` (one run of every task), `results/v1/parity_pilot/` (does the proxy
distort results?) and `results/v1/nsys_pilot/` (what the GPU does, kernel by kernel).

## Baseline results (M4.1)

The whole suite (`qwen3.6` at 32k context, 10 trials per task, seeded random order, RTX 3080; 140 measured plus
28 uninstrumented parity trials) was run twice:

| Run | Setup | Where |
|---|---|---|
| first | OpenCode as installed: its prompt included a skills list read from the author's `~/.claude` (a leak found by the prompt-budget analysis) | `results/v1/baseline/` |
| **isolated** (the reference baseline from here on) | same suite, OpenCode isolated with `OPENCODE_DISABLE_CLAUDE_CODE=1` | `results/v1/baseline_isolated/` |

Everything below is from the isolated run unless stated; the side-by-side comparison with confidence intervals is
`results/v1/baseline_isolated/COMPARISON.md`, the full report `RESULTS.md` / `report.html` next to it, and
`python -m analysis.figures --run <dir>` regenerates the figures.

**Reliability: one run is not enough.** 99 of 140 trials passed (71 %, 95 % CI 63-78 %; first run 74 %, difference
-3.6 pp, CI -13.9 to +6.9: no measurable change, but a drop of more than 5 pp cannot be excluded). Only four tasks
passed every time (`bugfix_01`, `coding_04`, `comms_07`, `longctx_06`); `c_ringbuf_03`, `coding_06` and
`research_08e` (which needs the 64k context window, deliberately run at 32k) passed 3 of 10. The earlier one-run
calibration saw 11 of 14 tasks pass; at n = 10 only four are reliable. With n = 10 the intervals are wide (8/10 is
49-94 %), and no single task differs significantly between the two runs.

![Pass rate per task](results/v1/baseline_isolated/figures/pass_rate_by_task.svg)

**Removing the leaked prompt block made trials about 42 % faster.** Pooled over the 14 tasks and normalised per
task, the median time ratio isolated/first is 0.58 (95 % CI 0.52-0.63, Mann-Whitney p < 1e-19); the mean time per
trial fell from 123 s to 97 s (-22 %, the long `c_ringbuf_03` and `research_08e` trials did not change). The first
prompt shrank from 10.1k to 7.3k tokens, and the cause of most of the speed-up is not the 2.8k tokens: in the first
run every trial started with a cold prefill (about 40 s at 253 tokens/s) because no prefix was reused between
trials, in the isolated run 130 of 140 first calls took about 2.7 s. The tokens alone would explain about 11 s per
trial of the 26 s saved; the rest is prefix caching, whose mechanism is not yet confirmed (plan item P12). The
effect depends on trial order: the cold first calls (10 of 140; all 9 that follow another trial) involve the two tasks with
their own tool sets (`comms_07`, `research_08e`), and a one-off session in a new project would still pay about 29 s of cold prefill.

**Where the time goes (isolated run).** 33 % of wall-clock is prefill (54 % in the first run), **60 % is decode**
(43 %), tool execution is 3.4 % (5.7 % for code generation) and harness overhead 3 %; nothing is unattributed. Decode
now dominates most task types; long-context and log-triage tasks are still prefill-heavy (71 % / 64 %).
The prompt itself is 68 % tool schemas (about 5k tokens: `bash` 1.3k, `task` 0.9k, `todowrite` 0.6k), 29 % OpenCode's
base system prompt and 2 % skills. Details: `promptbudget.md` in each run directory.

![Where the time goes](results/v1/baseline_isolated/figures/time_by_task_type.svg)

**What this changes about the earlier pilots**
- The "about 75 % prefill" from the two-trial pilot does not hold: 54 % suite-wide in the first run, 33 % once the
  prompt is isolated (and 11-71 % by task type).
- Tool execution is small (1-3 %): the "GPU idle while tools run" story does not apply to this suite.
- NVML reports the GPU as non-idle for about 90 % of wall-clock, but that is "any kernel in the sample window"; the
  Nsight pilot (two trials) found real kernel-active time during decode to be far lower, so GPU *utilisation* and
  GPU *efficiency* must not be conflated. The Nsight capture has not been repeated at baseline scale.
- Prefill levers (trimmed prompts) now matter less for back-to-back runs, decode levers more (see the plan, M5).

**Verification claims (`results/v1/*/claims.md`, `python -m analysis.claims --run <dir>`).** On the six
code-producing tasks (code generation, bug fix, refactor; 60 trials per run) the agent ends with an explicit claim
("all tests pass", "verified", "compiles cleanly") in 25 trials (42 %, CI 30-54 %) in both runs. In 24 of these 25 it had
run code that exited 0 before saying so, and nothing was edited afterwards; once it claimed without running anything
(`coding_04`). So the v0 observation, "claims tests pass without running them", is not reproduced in this setup: it is
rare. What does occur is *over-claiming from weak self-checks*: 5 of 60 trials (8 %, CI 4-18 %; 2 of 60 in the first
run) end with a claim although the hidden verifier fails, mostly `c_ringbuf_03` (3 of its 3 claims, all after the agent's
own compile-and-run succeeded; the hidden tests use sanitizers and edge cases the agent did not try).

**16k context (M4.2, `results/v1/m4_2/`).** At 16k the pass rate is 65 % (159 trials) against 72 % for a 32k reference on
the same server (80 trials): no significant overall loss (difference -7.1 pp, CI -18.6 to +5.6), but a clear loss on
tasks whose working context outgrows the window (`logtriage_04` 1/10 vs 4/5; its prompts reach 13-15.6k tokens and the
oldest tool results are dropped; the two long-context tasks 0/3). The earlier v0 note that 16k regresses on
fizzbuzz-style tasks is not reproduced. A runner bug found during this run (confinement gave every trial its own temp
path, which OpenCode prints in its first tool and which broke the prompt cache) inflates all M4.2 trial times by about
28 s; success rates are unaffected, and later runs have the fix.

**GPU activity (M4.3, `results/v1/m4_3/`).** 20 Nsight captures over 10 task categories show that the GPU is actually
working **11.7 % of the wall-clock** (8.9-13.0 % per category; about 9 % of prefill and 10-17 % of decode time), while NVML
reports 37-83 % for the same windows. With about two thirds of the expert weights in system memory the GPU computes in short
bursts and waits for the CPU: the processor is the bottleneck, in prefill and in decode.

**A smaller second model (`qwen3:8b`, `results/v1/q3_8b_core_n3_confined/`).** Fully on the GPU (q8_0 KV cache), it is faster per
trial but passes 24 % of the core-suite trials (10/42) against 66 % for `qwen3.6` under identical conditions (difference -42 pp,
CI -57 to -23); it often stops early without doing the work. The larger model's partial CPU offload costs time but buys a pass
rate 2.7 times as high.

**Sampling temperature (`results/v1/m5_temperature/`).** Lowering the temperature from the model default (1.0) to 0.3 or 0.6 does not
change the pass rate detectably (60 % / 60 % / 69 %, 42 trials each, intervals overlap) and does not make repeated trials more
consistent (cells with mixed outcomes 6 / 9 / 4 of 14): the spread between runs comes from the agent's path, not from sampling.

**Speculative decoding (`results/v1/m5_specdecode/`).** Nine settings (multi-token prediction at draft lengths 2-4, n-gram speculation, no speculation twice) were replayed on 49 recorded agent calls: none is reliably faster (0.77x to 1.04x against 37.8 tok/s) although n-gram drafts are accepted up to 84 % of the time, because checking several tokens at once reads many more CPU-resident expert weights. Speculation also changes the generated text (identical replies: 29-33 % for MTP, 71-82 % for n-gram, 100 % for a repeated run without speculation). Keep it off.

**Agent configuration (`results/v1/m5_agents/`).** A custom OpenCode agent replaces the built-in 9.6k-character system prompt, so the test used a short-prompt control. On nine code-writing tasks (n = 3, 81 trials) the short prompt alone leaves the pass rate unchanged (59 % against 56 %), while a short prompt plus a mandatory "run it, test an edge case, report what you ran" step reached 74 % with fewer false claims (1 against 3 and 5). The screen was not significant (CI of the gain -7 to +41 points), and the confirmation (`results/v1/m5_agents_confirm/`, four tasks that moved, n = 10, 80 trials) did not reproduce the gain: 45 % with the verification agent against 48 % with the default (CI of the difference -23 to +19). The verification agent made fewer false claims (2 against 7, suggestive only) but timed out 5 times against none on the long tasks. The default agent stays. A re-check of the `comms` agent (`results/v1/m5_comms_recheck/`, n = 10, 40 trials) did not reproduce its earlier call reduction (median 11 against 12 and 2.5 against 3 calls on the two comms tasks) and found no success benefit. A prompt-replay of trimmed tool schemas (`results/v1/m5_promptreplay/`) cuts the cold first call by 26-34 % (about 8-12 s per new session) and nothing once the cache is warm.

**Caveats.** Instrumentation parity is still inconclusive by its rule, but now with a clean same-run comparison (`results/v1/p10_parity/`: 96 proxied vs 96 direct trials of five
low-variance tasks): uninstrumented/instrumented time ratio 1.04, 95 % CI 0.998-1.094 against a ±5 % tolerance, per-task medians within 2-7 %, success 58 % vs 65 %. There is no
sign that the proxy slows anything down (earlier measurements 1.06 and 0.87 scatter around 1), but the overhead is bounded only to about ±5-9 %. One parity trial hung waiting for Ollama and
was recorded as a timeout; it was kept in `results/v1/baseline_isolated/hung_trials/`, re-run after a stall detector was
added to the runner (a trial with no events, an idle GPU and no working child process for 5 minutes is now a retried
harness failure, `--stall-s`), and the re-run passed.

## Repository map

| Path | Contents |
|---|---|
| `harness/` | the measurement tooling: `runner` (seeded, resumable trials), `proxy` (passive metrics proxy), `telemetry` (NVML sampler), `trace` and `decompose` (per-step timeline and time breakdown), `verifiers` + `sandbox` (automatic pass/fail), `taskspec`, `replay`, `promptreplay` (time prompt variants from captured requests), `server` (per-configuration Ollama server settings), `vocabcheck` + `ggufmeta` (draft-model compatibility), `nsys`, `manifest` |
| `analysis/` | `stats` (Wilson, bootstrap, Mann-Whitney, Holm), `parity`, `report` + `svg` (Markdown and inline-SVG HTML report), `figures` (standalone SVG charts), `compare_runs`, `claims` (verification-claim check), `determinism` |
| `tasks/` | the 18 task specs (YAML): prompts, tool permissions, timeout, verifiers (14 original, 4 harder ones added after the baseline) |
| `fixtures/` | task inputs (`tasks/`), hidden verifier assets (`verify/`), known-good/bad examples (`golden/`), recorded traces (`traces/`) and the replay runs with committed golden outputs (`replay/`) |
| `configs/` | run configurations: `baseline_32k.yaml` and the `sweep/` set (context size, KV cache type, flash attention, fit margin) |
| `agent-config/` | OpenCode agents, the mock email/calendar MCP tools, Modelfiles, SearXNG settings |
| `benchmark/` | the v0 scripts, kept for reference |
| `results/` | v0 reports (`results/v0/`, `report-*.md`) and v1 data (`results/v1/`) |
| `tests/` | unit, golden and end-to-end tests (no GPU needed) |

## Quickstart

Requirements: Python 3.12, [OpenCode](https://opencode.ai), Ollama with the `qwen3.6-32k` model
(see `agent-config/Modelfile`), an NVIDIA GPU with drivers for the live runs. SearXNG
(`docker start searxng`, see below) is only needed for the research task, Nsight Systems only for `harness.nsys`.

```bash
python3 -m venv .venv && . .venv/bin/activate
pip install -e ".[dev,gpu]"      # "gpu" adds the NVML bindings and the MCP library the mock comm-tools server needs
ruff check . && pytest           # unit, golden and replay tests; no GPU required
```

**Live run** (needs the GPU box):

```bash
OUT=results/v1/baseline
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 &   # metrics proxy
python -m harness.runner run --configs configs/baseline_32k.yaml --n 10 --ignore-task-trials --seed 1 \
    --out $OUT --scratch /tmp/agent_opt_scratch --parity-fraction 0.2    # resumable: re-run the same command to continue
python -m harness.runner status --out $OUT
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT   # per-step timeline, time breakdown
python -m analysis.report --run $OUT                                          # RESULTS.md + report.html
python -m analysis.figures --run $OUT                                         # standalone SVG charts
```

Use `--tasks <id> ...` to run a subset and `--dry-run` to see the randomized trial order first. Tasks belong to
named suites (`suite:` in the task YAML): `core` (the 14 tasks that can still show a difference; about 1.5 h at
`--n 3`) and `guard` (4 tasks that always passed; regression guards that carry `trials: 3`). `--suite core` selects a
suite; a task's own `trials` overrides `--n` unless `--ignore-task-trials` is given (the baseline runs above used
n = 10 for every task). `status --by-task` shows progress per task, and every trial record keeps the previous
trial's id and task (`prev_trial_id`, `prev_task`): the randomized order is back-to-back, so a trial's first call can
be served from the previous trial's prompt cache, which analyses can use as a covariate. Run
`python -m harness.manifest` to record the hardware/software manifest.

**Confinement.** OpenCode runs with every permission allowed and the rights of the user, and a weaker model once
searched the whole filesystem and edited the repository's fixtures. The runner therefore starts every trial under
Linux Landlock (`harness/landlock.py`, no root needed): a trial can read system paths and write only inside its own
trial directory (`TMPDIR` is set there); the repository (including the hidden verifier files), the home directory and
other trials are not accessible. `--no-confine` turns it off.

**Server settings per configuration.** A config with a `server:` section (`num_ctx`, `kv_cache_type`,
`flash_attention`, `num_parallel`, `fit_target_mib`; see `configs/sweep/` and `harness/server.py`) runs on a private
`ollama serve` that the runner starts with exactly those settings when the config's block of trials begins, points
the proxy and the parity trials at it, and stops afterwards; what the server actually did (layers offloaded, KV
cache, flash attention) is read back from its log into `platform.yaml`. Configs with different server settings run
in separate blocks (seeded order; trials are still shuffled inside a block). `python -m harness.server survey
--configs configs/sweep/*.yaml --out <dir>` starts each server once without running trials (results:
`results/v1/server_survey/`).

**Re-derive results from recorded runs** (no GPU, no model):

```bash
python -m harness.replay --run fixtures/replay/parity_pilot --out /tmp/replay
diff -r /tmp/replay fixtures/replay/parity_pilot/expected     # byte-for-byte equal to the committed golden output
```

Other entry points: `python -m harness.taskspec` (validate the task specs), `python -m harness.verifiers <task> <workdir>`
(re-run a task's checks), `python -m harness.nsys capture --task <id> --out <dir> --scratch <dir>` (Nsight Systems).

Notes: `opencode run` blocks unless stdin is `/dev/null` (the runner handles this); the runner uses an isolated OpenCode
config and does not touch your global one.

---

## First pass (v0): the OpenCode + Ollama proof of concept

### Project Scope and Setup

The target user is a developer using a local agent for coding and some research/benchmarks,
prioritizing privacy and offline capability over raw throughput. The original scope called
for a `Panther Lake` system with 64 GB unified memory and an Intel `Arc B390` GPU. That
hardware wasn't available to me for this phase, so I substituted my home desktop instead;
a deliberate, documented deviation (see `DIRECTION.md` for how the findings here are expected
to generalize, and where they won't) - with the following features:

| Feature | Description                                              |
|---------|----------------------------------------------------------|
| CPU     | 11th Gen Intel Core i7-11700K @ 3.60GHz                  |
| Memory  | 64GiB, 4 x 16GiB DIMM DDR4 Synchronous 2667 MHz (0.4 ns) |
| Disk    | 1TB NVME (64 bits, 33MHz); 2TB SSD                       |
| GPU     | Nvidia GeForce RTX 3080 - 10G VRAM                       |


### Model selection

Initially, I used `qwen2.5:14b-instruct` but this model doesn't fully fit into my 3080's 10GB VRAM,
so a large chunk was running on CPU. This led to sluggish performance, about 4.47 tokens per
second. I should expect more than 40tok/s if the model fits into VRAM.

The model `qwen2.5:14b-instruct-q4_K_M` performed better on the sample prompt `produce a C-code
function to calculate the square root of a floating-point number` and I saw it processing the prompt
1423.73 tok/s and generating 31.63 tok/s. 

> The smaller model first tried to just generate a wrapper and use math.h and I had to prompt again
> for an iterative solution, this did not happen on the `qwen2.5:14b-instruct` model. Clearly the
> quality of the smaller model required more explicit prompting.


ollama launch opencode
> opencode was unable to run any tools calls, so I extended the context window to 32k tokens

Use this `Modelfile` to extend the context window:
```
FROM qwen3.6
PARAMETER num_ctx 32768
```

Then build and launch it:

```bash
ollama create qwen3.6-32k -f Modelfile
ollama launch opencode --model qwen3.6-32k
```

This fixed the tools call. However, the initial baseline has no retrieval tool; the model attempts
to answer from training data alone, producing no verifiable citations.  A search tool will used in other test cases.

#### Test cases:

> The suite has since grown to 14 executable tasks in `tasks/` (fixtures in `fixtures/`). The six coding/reasoning
> prompts below are tasks 01 to 06 of it, with the output file names made explicit.

I defined four coding prompts and two language/logic reasoning prompts

1. In working directory `/home/npalmass/work/OpenCode/claude-files`, review the file `my_sqrt.c` and check if the function correctly calculates the square root of a float number.
   >  expected is a missing check for negative numbers, tolerance does not scale, potential infinite loop
2. Read the requirements in the file `fizzbuzz.md` and create C-code for it. Write to file `fizzbuzz.c`
   >  expected is valid C-code and current execution that matches the given test case
3. Follow requirements in `compress.md` and produce the C-function
   > expected is a simple string compressor function to test manual memory/pointer handling
4. Write the bash script requested in `dir_stats.md`
   > expected is a script that shows content stats of a directory path
5. What is the current scientific consensus for the MMR vaccine being linked to autism?
   > it turns out that OpenCode can follow links and fetch content, but at this time it has no access to search engine so the expected output of this question is all from training
6. Explain in less than 150 words why lowering corporate tax rates will not boost long-term job growth
   > designed to test reasoning from training data alone

### Methodology

> This is an AI assisted summary of the setup issues encountered during instrumentation:

Per-query performance metrics (time-to-first-token, input/output tok/s, and GPU/CPU/RAM utilization) were captured via a lightweight logging proxy inserted between OpenCode and Ollama's OpenAI-compatible endpoint, with a background sampler recording system resource utilization at one-second intervals for later correlation against each request's timestamps. This approach proved to carry a real, non-obvious risk that is worth stating explicitly rather than treating as a solved implementation detail: the instrumentation layer itself was, over the course of development, the source of multiple failure modes that were difficult to distinguish from genuine model or agent-harness problems. A Content-Length mismatch in the proxy caused every request to fail outright after the request body was modified; a missing SSE blank-line delimiter caused generation to complete successfully while the client received no usable stream, producing silent, errorless non-output; and - most subtly - injecting a `stream_options` flag to obtain exact token counts altered how Ollama's compatibility layer split model output between reasoning and final-answer fields, causing well-formed responses to appear as empty, failing turns purely as an artifact of the measurement setup. Each of these was initially indistinguishable from a real model or harness limitation, and each cost significant time to isolate precisely because the instrumentation was positioned to observe the same signal it could also corrupt. The methodological implication is that any benchmark relying on a custom instrumentation or proxy layer must budget for this class of risk explicitly: a proxy in the request path is not a passive observer, and apparent model failures, degraded output, or missing results should be cross-checked against an uninstrumented baseline before being attributed to the model or agent harness under test.

### Run test cases: ollama launch opencode:

> **Historical.** This is the manual v0 procedure. The scripts below now live in `harness/`
> (`python -m harness.proxy` etc.) and the whole loop is automated by `python -m harness.runner`;
> see the quickstart above. `join_metrics.py` was never committed in v0 and has been reconstructed as
> `harness/join_metrics.py`.

Setup instrumentation:
1. Set up proxy to track the timing of API calls for each test.
```
python3 metrics_proxy.py        # terminal 1 - leave running
```
2. Start the sampler to record usage stats.
```
rm samples.csv ; python3 sampler.py   # terminal 2 - leave running
```
3. Record the test name before each run so reports are grouped by test.
```
echo "coding_01_my_sqrt_review" > current_task.txt
```

4. Launch opencode in a terminal:
> I am deliberately launching opencode from ollama, to inherit its configuration and avoid the issues encountered with the missing generated response (See above summary)
```
export OLLAMA_HOST=127.0.0.1:11435  # route through instrumentation proxy
ollama launch opencode --model qwen3.6-32k`
```

### Generate the initial report

This will generate a Markdown report for each test showing TTFT, throughput, and resource utilization.

```bash
python3 join_metrics.py
python3 report.py
```

----

### First pass: Context window / GPU-layer-fit experiment

I attempted a performance improvement to reduce num_ctx (e.g., 16k instead of 32k) to make
sure the quantization better fits the GeForce RTX 3080 10GB VRAM with less need to offload
inference to CPU.

Create a new model:

```
ollama create qwen3.6-16k -f Modelfile2
ollama launch opencode --model qwen3.6-16k
```

See test cases 1-6 above.


### Second Pass: enable comm-tools to send emails and calendar appointments

#### Test cases:

I was using the following prompts to simulate a common workflow:

"Use the send_email tool to draft an email to `alice@example.com` with subject \"Project update\" and body \"The benchmark suite is on track.\" Do not send it until I explicitly approve it."  
"show the draft"  
"Looks good, send it."  
"Use the create_event tool to schedule \"Benchmark review\" from 2026-08-10T14:00:00 to 2026-08-10T14:30:00 with attendees `alice@example.com` and `bob@example.com`. Confirm with me before creating it."  
"Send that same email again using send_email, exact same recipient, subject, and body."  
"ok, create the event now"  


### Third Pass: enable Web search, add to MCP for OpenCode

I selected a self-hosted SearXNG instance (the actual search engine), and an MCP bridge that
exposes it to OpenCode as a tool. SearXNG's default config only returns HTML, so I added a
custom `settings.yml` to explicitly enable JSON format.

```bash
mkdir -p ~/work/searxng-config
cat > ~/work/searxng-config/settings.yml << EOF
use_default_settings: true
search:
  formats:
    - html
    - json
server:
  secret_key: "AgenitcPoniesinthewinter"
  limiter: false
outgoing:
  request_timeout: 10.0
  max_request_timeout: 15.0
EOF
```

Then start the docker on port 8090 since port 8080 is taken by Ollama web UI

```bash
docker run -d --name searxng -p 8090:8080  \
  -v ~/work/searxng-config/settings.yml:/etc/searxng/settings.yml:ro  \
  searxng/searxng
```

> use `curl "http://localhost:8090/search?q=test&format=json"` to verify it's serving JSON correctly.

Once it is running, we can add it as MCP bridge to OpenCode:

```bash
opencode mcp add
```

When prompted:  
- Name: `searxng`  
- Command: `npx -y mcp-searxng`  

Since it's not letting us enter the environment, we need to edit the config file manually as follows:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "permission": "allow",
  "provider": {
    ... 
 },
  "mcp": {
    "searxng": {
      "type": "local",
	"command": ["npx", "-y", "mcp-searxng"],
	"environment": {
	    "SEARXNG_URL": "http://localhost:8090"
	},
	"enabled": true
    }
  }
}

```

This command to confirm it is registered correctly:

```bash
opencode mcp list
```

Expected output:
```
┌  MCP Servers
│
●  ✓ searxng connected
│      npx -y mcp-searxng
│
●  ✓ comm-tools connected
│      python3 /home/npalmass/work/OpenCode/claude-files/comm-tools/comm_tools_mcp.py
│
└  2 server(s)
```

#### Test case for deep research using web_search

The test case requires the model to search the web for current research and opinions and
line up verifiable citations.

8. Does neuroscience research on decision-making - such as Libet-style readiness-potential
   experiments - support the conclusion that free will is an illusion, or is that conclusion
   scientifically and philosophically contested? Summarize the strongest evidence and
   arguments on both sides, citing specific studies.
   > Expected: retrieval of multiple named studies (not just Libet's original), correct
   > representation of methodological critiques (e.g. Schurger et al.'s challenge to the
   > readiness-potential interpretation), and citations traceable to real fetched sources
   > rather than confident-sounding but ungrounded claims

After observing incorrect citations, a new `research` agent was defined, this showed some
improvement but systemic model-based issues remained (see `results/README.md`).
