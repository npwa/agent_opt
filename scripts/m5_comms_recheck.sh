#!/bin/bash
# H6 re-check of the comms agent: comms agent (sw_32k) vs default agent (sw_32k_nocomms) on comms_07 and comms_12, n = 10 each -> 40 trials, about 2 h.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt H6 comms" scripts/m5_comms_recheck.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_comms_recheck
TASKS=(comms_07_confirm_send comms_12_timezone_slot)
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs configs/sweep/sw_32k.yaml configs/sweep/sw_32k_nocomms.yaml \
    --tasks "${TASKS[@]}" --n 10 --ignore-task-trials --seed 3 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k \
    && python -m analysis.figures --run $OUT && python -m analysis.claims --run $OUT && python -m analysis.consistency --run $OUT || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
