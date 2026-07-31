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
