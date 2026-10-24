#!/bin/bash
# H6 confirmation: default agent vs verification-step agent (no lean arm) on the four tasks where the n = 3 screen showed a
# difference or a loss (bugfix_09, c_ringbuf_03, coding_10, coding_06), n = 10 each -> 80 trials, about 4-5 h. A new run seed gives
# new trial seeds. Caveat to carry into the write-up: the tasks were chosen because they moved in the screen (regression to the mean).
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt H6 confirm" scripts/m5_agents_confirm.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_agents_confirm
TASKS=(bugfix_09_booking_slots c_ringbuf_03_ringbuffer coding_10_wcx_constrained coding_06_tax_rates_lan)
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs configs/sweep/sw_32k.yaml configs/sweep/sw_32k_verify.yaml \
    --tasks "${TASKS[@]}" --n 10 --ignore-task-trials --seed 2 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k \
    && python -m analysis.figures --run $OUT && python -m analysis.claims --run $OUT && python -m analysis.consistency --run $OUT || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
