# Take-Home Project: Local Agent Optimization with OpenCode

## Initial Assumptions for this Project

The target user is a developer using a local agent for coding and some research/benchmarks,
prioritizing privacy and offline capability over raw throughput. The initial assignment was
to use `Panther Lake` system with 64 GB unified memory and Intel `Arc B390` but after entering
CPM, I no longer had access to this hardware and relied on my home desktop with the
following features:

|Feature | Description|
|------|-
|CPU   | 11th Gen Intel Core i7-11700K @ 3.60GHz |
|Memory| 64GiB, 4 x 16GiB DIMM DDR4 Synchronous 2667 MHz (0.4 ns) |
|Disk  | 1TB NVME (64 bits, 33MHz); 2TB SSD |
|GPU   | Nvidia GeForce RTX 3080 - 10G VRAM |


## Model selection

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

### Test cases:

I defined four coding prompts and two language/logic reasoning prompts

1. In working directory `/home/npalmass/work/OpenCode/claude-files`, review the file `my_sqrt.c` and check if the function correctly calculates the square root of a float number.
   >  expected is a missing check for negative numbers, tolerance does not scale, potential infinite loop
2. Read the requirements in the file `fizzbuzz.md` and create C-code for it. Write to file `fizzbuzz.c`
   >  expected is valid C-code and current execution that matches the given test case
3. Follow requirements in `compress.md` and produce the C-function
   > expected is a simple string copressor function to test manual memory/pointer handling
4. Write the bash script requested in `dir_stats.md`
   > expected is a script that shows content stats of a directory path
5. What is the current scientific consensus for the MMR vaccine being linked to autism?
   > it turns out that OpenCode can follow links and fetch content, but at this time it has no access to search engine so the expected output of this question is all from training
6. Explain in less than 150 words why lowering corporate tax rates will not boost long-term job growth
   > designed to test reasoning form training data alone

## Methodology

> This is an AI assisted summary of the setup issues encountered during instrumentation:

Per-query performance metrics (time-to-first-token, input/output tok/s, and GPU/CPU/RAM utilization) were captured via a lightweight logging proxy inserted between OpenCode and Ollama's OpenAI-compatible endpoint, with a background sampler recording system resource utilization at one-second intervals for later correlation against each request's timestamps. This approach proved to carry a real, non-obvious risk that is worth stating explicitly rather than treating as a solved implementation detail: the instrumentation layer itself was, over the course of development, the source of multiple failure modes that were difficult to distinguish from genuine model or agent-harness problems. A Content-Length mismatch in the proxy caused every request to fail outright after the request body was modified; a missing SSE blank-line delimiter caused generation to complete successfully while the client received no usable stream, producing silent, errorless non-output; and — most subtly — injecting a `stream_options` flag to obtain exact token counts altered how Ollama's compatibility layer split model output between reasoning and final-answer fields, causing well-formed responses to appear as empty, failing turns purely as an artifact of the measurement setup. Each of these was initially indistinguishable from a real model or harness limitation, and each cost significant time to isolate precisely because the instrumentation was positioned to observe the same signal it could also corrupt. The methodological implication is that any benchmark relying on a custom instrumentation or proxy layer must budget for this class of risk explicitly: a proxy in the request path is not a passive observer, and apparent model failures, degraded output, or missing results should be cross-checked against an uninstrumented baseline before being attributed to the model or agent harness under test.

## Run test cases: ollama launch opencode:

Setup instrumentation:
1. Set up proxy to track the timing of API calls for each test.
```
python3 metrics_proxy.py        # terminal 1 — leave running
```
2. Start the sampler to record usage stats.
```
rm samples.csv ; python3 sampler.py   # terminal 2 — leave running
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

## Generate the initial report

This will generate a Markdown report for each test showing TTFT, throughput, and resource utilization.

```bash
python3 join_metrics.py
python3 report.py
```

----

## First pass: Quantization / GPU-layer-fit experiment

I attempted a performance improvement to reduce num_ctx (e.g., 16k instead of 32k) to make
sure the quantization better fits the GeForce RTX 3080 10GB VRAM with less need to offload
inference to CPU.

Create a new model:

```
ollama create qwen3.6-16k -f Modelfile2
ollama launch opencode --model qwen3.6-16k
```

See test cases 1-6 above.


## Second Pass: enable comm-tools to send emails and calendar appointments

### Test cases:

I was using the following prompts to simulate a common workflow:

"Use the send_email tool to draft an email to alice@example.com with subject \"Project update\" and body \"The benchmark suite is on track.\" Do not send it until I explicitly approve it."  
"show the draft"  
"Looks good, send it."  
"Use the create_event tool to schedule \"Benchmark review\" from 2026-08-10T14:00:00 to 2026-08-10T14:30:00 with attendees alice@example.com and bob@example.com. Confirm with me before creating it."  
"Send that same email again using send_email, exact same recipient, subject, and body."  
"ok, create the event now"  


## Third Pass: enable Web search, add to MCP for OpenCode

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
  proxies:
    all://:
      - http://proxy-dmz.intel.com:911
  extra_proxy_timeout: 10
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

### Test case for deep research using web_search

The test case requires the model to search the web for current research and opinions and
line up verifiable citations.

8. Does neuroscience research on decision-making — such as Libet-style readiness-potential
   experiments — support the conclusion that free will is an illusion, or is that conclusion
   scientifically and philosophically contested? Summarize the strongest evidence and
   arguments on both sides, citing specific studies.
   > Expected: retrieval of multiple named studies (not just Libet's original), correct
   > representation of methodological critiques (e.g. Schurger et al.'s challenge to the
   > readiness-potential interpretation), and citations traceable to real fetched sources
   > rather than confident-sounding but ungrounded claims

Initial result in `research_08_free_will` shows that the prompt merely recalls from training
and even after explicitly asked to produce verifiable references, the citations still blend
with the original training data recalled references. So the search happened, but citation
provenance didn't survive the synthesis step. The model has no mechanism distinguishing "I
confirmed this via a tool call this turn" from "I recalled this from training" — both get
rendered in the same authoritative format.

A new `research` agent was defined, this showed some improvement but systemic model-based
issues remained (See results section).
