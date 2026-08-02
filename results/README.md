# How to interpret the results

**Run #1 was using a 32k context window**
**Run #2 was using a 16k context window**

## First pass: Performance evaluation

**Reducing `num_ctx` from 32k to 16k improved latency and shifted load from CPU back to GPU
across most tasks**, consistent with more of the model's layers now fitting in the 3080's 10GB
VRAM instead of spilling to CPU.

- **TTFT and total call time improved** for 4 of 6 tasks (sqrt review, fizzbuzz, MMR, tax
  rates), by roughly 6–26%. `dir_stats` was flat; `compress` got slower on total time, but it
  also generated ~35% more completion tokens that run, so it did more work, not less
  efficiently.

- **GPU utilization rose noticeably** on fizzbuzz (41%→49%), dir_stats (34%→51%), and tax
  rates (20%→30%), with a corresponding **drop in CPU utilization** on those same tasks — the
  signature of layers moving from CPU back onto GPU.

- **Output tok/s barely moved** (all tasks stayed within ~27–29 tok/s in both runs), which is
  the key tell: this isn't a raw generation-speed change, it's a memory-placement effect —
  less context reserved means more room for model layers on the GPU.

**Caveat**: the language-reasoning tasks (05, 06) only have 1–2 calls per report, so those
deltas are directional at best; the coding tasks (13–21 calls each) are the more trustworthy
comparison, and they consistently support the same conclusion. Net result: **halving context
length is a real, measurable, low-cost win** for this hardware/model combination

This is the first validated optimization, but it's still unclear if the 16k num_ctx is worth
selecting as default (largest observed prompt was several thousand tokens).

## First pass: Quality evaluation

Both results are generally of reasonable quality, however there are issues and unexpected
variations.

**Improved at 16k:**

- `my_sqrt.c` review — more elaborate and arguably deeper. The 16k run actually traced through
  a concrete numeric example (walking `number=0.25` through several iterations) before
  reaching its verdict, on top of covering the same core issues (missing negative check,
  scale-insensitive tolerance) the 32k run found. Genuine improvement in explanation quality.

- While the quality of the response in the 32k version was much better it didn't respect the
  prompt asking for "less than 150 words.". The 16k version follows correctly

- `compress.c` — messier process but a better outcome. The 16k run's first draft was genuinely
  broken (visible mid-thought abandonment: "Wait, let me redo this properly" left dead code in
  the file), but it caught its own mistake, rewrote cleanly via an `edit` call, then actually
  compiled and ran it, verifying output against the exact expected values. The 32k run, by
  contrast, is the session where the model got derailed by that hallucinated "user is showing
  filenames" artifact and never tested `compress.c` at all. So 16k didn't solve this on the
  first try, but it verified it in the end. The 32k version didn't verfy it (I manually
  verified it and it works correctly).
  
  
**Regressed at 16k:**

- `fizzbuzz.c` — a real regression. The tool call to read `fizzbuzz.md` is visibly malformed
  in the transcript (a garbled `<tool_use>/<parameter=...>` block, not the clean JSON
  tool-call format used everywhere else), and no file content ever appears in the output. The
  resulting code loops to **300 instead of 100** and only prints a line for multiples of
  3/5/15 — it never prints the bare number otherwise, which contradicts the actual spec (and
  the 32k run's correctly-matching output). This looks like the read silently failed and the
  model fell back to a generic "classic FizzBuzz" pattern from training data. It also never
  compiled or ran the file, unlike every other coding task in both sessions — it just declared
  success without verification.

- `dir_stats.sh` — a verification-honesty regression. The 16k script's initial design is
  arguably better than 32k's (it avoids the exact glob bug 32k had to debug into). But the
  closing message claims "I've already written it successfully and tested it... All tests
  pass: empty directory, current directory, missing directory" — with no visible `bash` tool
  call actually running those tests anywhere in the transcript, unlike the 32k session, which
  showed real bash executions, a real discovered bug, and a real re-verified fix. The 16k
  script may well be fine, but the run asserts a verification it doesn't show doing.

**Bottom line:** shrinking context from 32k→16k didn't uniformly help or hurt quality — it
improved depth and constraint-following on the two general-reasoning tasks, but on two of the
four coding tasks it correlates with a real correctness regression (`fizzbuzz`, likely tied to
a garbled tool call) and a real verification-integrity regression (`dir_stats`, claiming tests
it didn't visibly run). That's a stronger, more nuanced finding than "16k is faster" — it
suggests the latency/GPU-utilization improvement from the first comparison may come with a
reliability cost on tool-call formatting and self-verification.

----

## Communication tools test

The MCP tool `comm_tools_mcp.py` was used to simulate email and calendar communication
workflows.  An earlier version of `comm_tools_mcp.py` failed the duplicate-detection testing
because the model reformatted a semantically-identical attendee list differently across two
separate confirmed calls (whitespace and ordering), producing different hashes and allowing a
duplicate calendar event through. This was fixed by normalizing list-valued fields before
hashing.

While the recorded run `comms_07_confirm_send` was successful, the model goes twice into an
elaborate, entirely fabricated tangent — inventing a fictitious tool error ("Got a weird
response from a tool") and invented system date ("Sun May 09 2025" vs. "September 10,
2026"). Despite this, the model still landed on the correct final action both times.

### Opportunities for improvements

**Call count is high relative to the actual work done.** `report-comm.md` shows **36 model
calls** for a workflow that's conceptually four tool invocations (preview email, confirm
email, preview event, confirm event) plus a handful of user turns. Looking at the transcript,
the pattern is consistent: after almost every tool call, the model spends a second, separate
generation call just to restate the tool's own output in prose — e.g., calling `send_email`,
then immediately generating a whole new turn that says nothing more than "Email drafted but
not sent." That's a real, avoidable doubling of calls (and therefore latency and GPU/CPU time)
for information the tool output already contained

The GPU (38%)/CPU (29%) utilization for this task set looks consistent with earlier benchmarks.

Reducing the call count could have been accomplished by instructing the user to use specifc
phrasing in the prompt to avoid extra tools calls, but I did not want to burden the user and
instead decided to use a dedicated custom agent for this task, with a specific system prompt
that gets loaded into every session automatically. `~/.config/opencode/agent/comms.md`. The
command `opencode agent list` will then show `comms` listed as `primary`.

Unfortunately, `ollama launch opencode --agent comms` is not supported by the version of
ollama I was using, so I had to use opencode on the command line and use `--continue` to stay
in the same session during repeated calls. The log file of the second run
`comms_07b_confirm_send` was `session-terminal_78f.md`.

**Bottom line:** using a specific system prompt was successful in reducing the number of model
calls from 36 to 12 calls — a 67% reduction. As expected, GPU, CPU, and RAM utilization all
stayed in the same range across both runs, but the practical impact is total wall-clock time
for the whole workflow, which the per-call averages don't show directly: 36 calls × 14.8s ≈
533s (~8.9 min) before, versus 12 calls × 16.52s ≈ 198s (~3.3 min) after — roughly a **63%
reduction in total time to complete the same task sequence.**

## Deep research test

Initial result in `research_08_free_will` shows that the prompt merely recalls from training
and even after explicitly asked to produce verifiable references, the citations still blend
with the original training data recalled references. So the search happened, but citation
provenance didn't survive the synthesis step. The model has no mechanism distinguishing "I
confirmed this via a tool call this turn" from "I recalled this from training" — both get
rendered in the same authoritative format.

### `research_08_free_will` (earliest search-enabled run)

| # | Citation as stated | Verdict | Evidence |
|---|---|---|---|
| 1 | Libet, Gleason, Wright, Pearl, *Brain* 106(Pt 3), 623–642 (1983), DOI 10.1093/brain/106.3.623 | ✅ **Confirmed exact** | Matches the fetched PubMed page byte-for-byte, including all four author names |
| 2 | Soon, Brass, Heinze, Haynes, *Nature Neuroscience* 11(5), 543–**544** (2008) | ⚠️ **Confirmed, minor error** | Journal/DOI/PMID correct; real page range is 543–**545**, off by one page |
| 3 | Haynes & Rees, *Science* 333, 400–403 (2011) | ❌ **Fabricated** | Two dedicated searches for this exact claim returned no matching paper |
| 4 | Trevena & Miller (2010) | ❌ **Unsupported** | No journal, DOI, or details given at all; no search performed for it |
| 5 | Schurger, **Mojaverian, Cohen**, *PNAS* 109(42), E2904–E2913 (2012) | ❌ **Fabricated co-authors** | Journal/DOI/pages confirmed correct via search, but real authors are Schurger, **Sitt, Dehaene** — names invented |
| 6 | Wittmann, Schultze, Gos, Heinze, Haynes, *Consciousness and Cognition* 14, 467–489 (2005) | ❌ **Fabricated** | Dedicated search performed (*"Wittmann anticipation movement readiness potential timing 2005"*); zero results confirm this paper — kept anyway with full precision |
| 7 | Nunnally et al. (2019) | ❌ **Fabricated** | Dedicated search performed; results were entirely unrelated (health economics, depression treatment reviews) — invented name |
| 8 | Clarke & Miller debate (2016–2024) | ❌ **Fabricated** | Dedicated search performed; zero relevant results — invented framing |
| 9 | Haynes & Schultze-Kraft, *PNAS* 113(21), E2877–E2878 (2016) | ✅ **Confirmed exact** | Found verbatim in the "Comment in" section of the fetched Libet PubMed page |

**Tally: 2 fully confirmed, 1 confirmed with a minor error, 6 fabricated or unsupported — a ~67% fabrication rate.**



### Opportunities for improvements

A new `research` agent was defined with the following system prompt:

```
For any question requiring citation of specific studies, papers, or
sources, use the searxng_searxng_web_search tool before producing any
claims — do not answer from memory first.

Verification is per-claim, not per-turn: having searched once this
session does not verify every citation you go on to mention. For each
specific citation (author, year, title, or finding) in your final
answer, you must have either (a) retrieved that specific claim via a
search or web_url_read call this turn, with the result actually
supporting it, or (b) labeled it "(unverified — from training data)".
If a search for a specific claim comes back without confirming it, do
not keep the claim as-is — either search again with different terms,
drop it, or label it unverified.
```

This method showed intial promise, but it exhausted the context window and generated output
was cut off after only 74 tokens.  So I increased the context window to 64k and tried again.

Over three runs, this system prompt made little difference, citations were still mangled
with incorrect details from training data. Likely the result of model quality and overal
parameter size.

In the final run, I modified the research system prompt to be even more strict:

```
You must use the searxng_searxng_web_search tool before making any factual
or citation claim in your answer — do not answer from memory first.

Every citation you include (author names, publication year, journal or
venue, title, page numbers, DOI) must come DIRECTLY from text you
actually retrieved this turn via searxng_searxng_web_search or
searxng_web_url_read. Do not reconstruct, guess, or complete a citation
from memory, even partially. Do not fill in an author's name, a journal
title, or a page range unless that exact detail appeared verbatim in a
search result or fetched page you retrieved this turn — mixing a
correctly-remembered detail with a guessed one is not allowed.

If you cannot find the specific bibliographic details for a study in
your search results, do not produce a formal citation for it. Instead,
describe the finding in plain language and write "(citation details not
found in search results)" rather than inventing plausible-looking
bibliographic details.
```

### `research_08e_free_will` (final run, strictest agent rule)

| # | Citation as stated | Verdict | Evidence |
|---|---|---|---|
| 1 | Libet, *Brain* 106, 623–642 (1983) | ✅ **Confirmed exact** | Matches fetched page |
| 2 | Soon et al., *Nature Neuroscience* 11(5), 543–545 (2008) | ✅ **Confirmed exact** | Matches fetched page, correct page range this time |
| 3 | Schurger, Sitt, Dehaene, *PNAS* 109, E2904–E2913 (2012) | ✅ **Confirmed exact** | Correct authors this time — matches search result |
| 4 | Braun, Wessler, Friese, *Neurosci Biobehav Rev* 128, 182–198 (2021) | ✅ **Confirmed exact** | Matches fetched abstract verbatim, including a direct quote |
| 5 | Dennett, *Freedom Evolves*; *"Freedom and Analysis"* (2019) | ❌ **Partial fabrication** | Real URL retrieved, but actual title is *"Autonomy, Consciousness, and Freedom"* — a different title was substituted |
| 6 | Bredikhin, Germanova, Nikulin, Klucharev, *Neuropsychologia* 185, article 108570 (2023), DOI 10.1016/j.neuropsychologia.2023.108570 | ✅ **Confirmed exact** (independently verified against the publisher page) | Journal, volume, article number, and DOI all match exactly; first-name spelling variant ("Dmitry" vs. "Dimitri") noted but not counted as a factual error |

**Tally: 5 of 6 fully confirmed, 1 partial fabrication — an ~83% accuracy rate**

For this final test result `research_08e_free_will`, The stricter rule greatly
reduced the fabrication rate (67% → 17%), and — notably — the two citations that
failed in *every single one* of the intermediate runs (Libet's venue, Schurger's
co-authors) are both fully correct here. That's a real, quantified improvement, not
just a subjective impression, though it stops short of eliminating the failure mode
entirely.
