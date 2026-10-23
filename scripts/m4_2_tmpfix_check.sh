#!/bin/bash
# After the M4.2 finding that a per-trial TMPDIR breaks the cross-trial prefix cache (every first call cold):
# run a few cheap tasks on sw_32k with the fixed runner (constant shared temp dir) to confirm that first calls are
# warm again and to measure the time effect against the M4.2 reference run (same config, per-trial TMPDIR).
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m4_2/sw_32k_tmpfix
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
sleep 3
python -m harness.runner run --configs configs/sweep/sw_32k.yaml \
    --tasks coding_02_fizzbuzz_gen coding_05_scientific_lan coding_06_tax_rates_lan bugfix_01_moving_average repoqa_05_retry_policy comms_07_confirm_send \
    --n 4 --ignore-task-trials --seed 1 --out $OUT --scratch /tmp/agent_opt_scratch
kill $PROXY 2>/dev/null
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT \
    && python -m analysis.promptbudget --run $OUT 2>/dev/null; python -m harness.promptbudget --run $OUT
python -m harness.promptbudget --run results/v1/m4_2/sw_32k_ref --out results/v1/m4_2/sw_32k_ref
echo "=== tmpfix check done: $(date '+%F %H:%M:%S')"
