#!/bin/bash
# P10: instrumentation parity top-up. Instrumented (through the proxy) and uninstrumented (direct to the private Ollama) trials
# of the same cells in ONE run (same server, confinement, cache state, interleaved), on the lowest-variance tasks:
# coding_05 and coding_06 n = 30 (time CV 0.14-0.18), coding_01 / repoqa_05 / refactor_02 n = 12; --parity-fraction 1.0 gives as many
# uninstrumented trials as instrumented ones: 192 trials, about 2.3 h.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt P10" scripts/p10_parity.sh
#   scripts/p10_parity.sh smoke      # coding_05 n = 1 (2 trials): do both paths run?
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
MODE=${1:-full}
if [ "$MODE" = smoke ]; then
    OUT=results/v1/p10_parity_smoke; ARGS=(--tasks coding_05_scientific_lan --n 1)
else
    OUT=results/v1/p10_parity
    ARGS=(--tasks coding_05_scientific_lan coding_06_tax_rates_lan coding_01_my_sqrt_review repoqa_05_retry_policy refactor_02_rename_price
          --n 12 --n-for coding_05_scientific_lan=30 coding_06_tax_rates_lan=30)
fi
mkdir -p $OUT/proxy
docker start searxng > /dev/null 2>&1
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs configs/sweep/sw_32k.yaml "${ARGS[@]}" --parity-fraction 1.0 --ignore-task-trials --seed 1 \
    --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
