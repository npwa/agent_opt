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
| M4 | Baseline campaign: whole suite, n = 10, 32k vs 16k re-check | M4.1 done (32k baseline, 168 trials); M4.2 (16k) next |
| M5 | Lever experiments: context/KV size, prompt budget, speculative decoding, second model, agent configs | planned |
| M6 | Automated citation verifier and research evaluation | planned |
| M7 | Write-up (README, deck, results) | planned |

Earlier pilots: `results/v1/calibration/` (one run of every task), `results/v1/parity_pilot/` (does the proxy
distort results?) and `results/v1/nsys_pilot/` (what the GPU does, kernel by kernel).

## Baseline results (M4.1)

Whole suite, `qwen3.6` at 32k context, 10 trials per task, run in a seeded random order on the RTX 3080 (about
5.5 hours; no harness failures, no retries). 140 measured trials plus 28 uninstrumented parity trials. Everything
below is derived from `results/v1/baseline/`; the full report is `RESULTS.md` / `report.html` there, and
`python -m analysis.figures --run results/v1/baseline` regenerates the figures.

**Reliability: one run is not enough.** 104 of 140 trials passed (74 %, 95 % CI 66-81 %). Only four tasks passed
every time; most of the rest sit between 60 % and 90 %, and two tasks mostly fail. The earlier one-run calibration saw 11 of
the 14 tasks pass; at n = 10 only four are reliable.

![Pass rate per task](results/v1/baseline/figures/pass_rate_by_task.svg)

With n = 10 the intervals are wide (8/10 is 49-94 %), so only the extremes are clearly different from each other:
`coding_06_tax_rates_lan` (3/10, fails the word limit) and `research_08e_free_will` (1/10, needs the 64k context
window and was deliberately run at 32k) against the four 10/10 tasks. Runtimes vary too: `c_ringbuf_03` took
109 s to 975 s across its ten trials.

**Where the time goes.** Across all trials, 54 % of wall-clock is prefill (the model reading its prompt before
the first token), 43 % is decode, tool execution is 0.9 % and harness overhead 2.4 %; nothing is left unattributed.
The split depends strongly on the task type: long-context and reasoning tasks are 80 % prefill, code generation
is 60 % decode. The first LLM call of a session accounts for 72 % of all prefill time (about 39 % of total
wall-clock). That fits the roughly 10k-token tool/system prompt being processed in full on the first call and
served mostly from the prefix cache afterwards; M5 will confirm this per call from Ollama's `prompt_eval_count`.

![Where the time goes](results/v1/baseline/figures/time_by_task_type.svg)

**What this changes about the earlier pilots**
- The "about 75 % prefill" from the two-trial pilot does not hold suite-wide (54 %; 37-81 % by task type). It was
  a property of the short, read-heavy tasks in that pilot.
- Tool execution is negligible (about 1 %), confirmed: the "GPU idle while tools run" story does not apply to this suite.
- NVML reports the GPU as non-idle for 95 % of wall-clock, but that is "any kernel in the sample window"; the
  Nsight pilot (two trials) found real kernel-active time during decode to be far lower, so GPU *utilisation* and
  GPU *efficiency* must not be conflated. The Nsight capture has not been repeated at baseline scale.

**Caveats.** Instrumentation parity is still inconclusive (time ratio 1.06, 95 % CI 0.96-1.19, tolerance ±5 %,
only 28 parity trials, two per task; success rate is consistent: 74 % vs 68 %). The baseline covers one
configuration, so no comparison is made yet. The 32k vs 16k comparison and the re-check of the v0 claims are
M4.2 and not yet run.

## Repository map

| Path | Contents |
|---|---|
| `harness/` | the measurement tooling: `runner` (seeded, resumable trials), `proxy` (passive metrics proxy), `telemetry` (NVML sampler), `trace` and `decompose` (per-step timeline and time breakdown), `verifiers` + `sandbox` (automatic pass/fail), `taskspec`, `replay`, `nsys`, `manifest` |
| `analysis/` | `stats` (Wilson, bootstrap, Mann-Whitney, Holm), `parity`, `report` + `svg` (Markdown and inline-SVG HTML report), `figures` (standalone SVG charts) |
| `tasks/` | the 14 task specs (YAML): prompts, tool permissions, timeout, verifiers |
| `fixtures/` | task inputs (`tasks/`), hidden verifier assets (`verify/`), known-good/bad examples (`golden/`), recorded traces (`traces/`) and the replay runs with committed golden outputs (`replay/`) |
| `configs/` | run configurations (`baseline_32k.yaml`) |
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
python -m harness.runner run --configs configs/baseline_32k.yaml --n 10 --seed 1 \
    --out $OUT --scratch /tmp/agent_opt_scratch --parity-fraction 0.2    # resumable: re-run the same command to continue
python -m harness.runner status --out $OUT
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT   # per-step timeline, time breakdown
python -m analysis.report --run $OUT                                          # RESULTS.md + report.html
python -m analysis.figures --run $OUT                                         # standalone SVG charts
```

Use `--tasks <id> ...` to run a subset and `--dry-run` to see the randomized trial order first. Run
`python -m harness.manifest` to record the hardware/software manifest.

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
