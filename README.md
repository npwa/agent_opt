# Take-Home Project: Local Agent Optimization with OpenCode

## Initial Assumptions for this Project

The target user is a developer using a local agent for coding and some research/benchmarks,
prioritizing privacy and offline capability over raw throughput.

## Model selection

Initially, I used `qwen2.5:14b-instruct` but this model doesn't fully fit into my 3080's 10GB VRAM,
so a large chunk was running on CPU. This led to sluggish performance, about 4.47 tokens per
second. I should expect more than 40tok/s if the model fits into VRAM.

The model `qwen2.5:14b-instruct-q4_K_M` performed better on the sample prompt `produce a c-code
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
to answer from training data alone, producing no verifiable citations.  A search tool will be added later if needed.

## Test cases:

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
python3 metrics_proxy.py   # terminal 1 — leave running
```
2. Start the sampler to record usage stats.
```
rm samples.csv ; python3 sampler.py     # terminal 2 — leave running
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

## Quantization / GPU-layer-fit experiment

The first pass on performance improvement is to reduce num_ctx (e.g., 16k instead of 32k) to
make sure the quantization better fits the GeForce RTX 3080 10GB VRAM.  
Create a new model:

```
ollama create qwen3.6-16k -f Modelfile2
ollama launch opencode --model qwen3.6-16k
```
