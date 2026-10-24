#!/bin/bash
# M5.5 / H6: three agent arms on nine code-writing core tasks, n = 3 -> 81 trials, about 2.7 h.
#   sw_32k         default OpenCode agent (9.6k-character system prompt)
#   sw_32k_lean    short prompt, no verification text      (agent-config/agent/coding_lean.md)
#   sw_32k_verify  short prompt + "run it, test an edge case, report what you ran" (agent-config/agent/coding.md)
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt H6" scripts/m5_agents.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_agents
TASKS=(bugfix_09_booking_slots c_ringbuf_03_ringbuffer coding_02_fizzbuzz_gen coding_03_compress_gen coding_05_scientific_lan
       coding_06_tax_rates_lan coding_10_wcx_constrained refactor_02_rename_price repoqa_05_retry_policy)
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs configs/sweep/sw_32k.yaml configs/sweep/sw_32k_lean.yaml configs/sweep/sw_32k_verify.yaml \
    --tasks "${TASKS[@]}" --n 3 --ignore-task-trials --seed 1 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k \
    && python -m analysis.figures --run $OUT && python -m analysis.claims --run $OUT && python -m analysis.consistency --run $OUT || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
