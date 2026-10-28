#!/bin/bash
# H1 success guard: does removing four unused tool schemas (sw_32k_trimtools) change the pass rate? Eight short non-MCP tasks, n = 5.
# Only the trimmed arm is run here; the control is the sw_32k arm of results/v1/m5_trimtools_skill_only (same tasks, n = 5, seed 5, same
# server settings), whose trimmed arm turned out to remove only the `skill` tool (config bug, see that directory's README).
# Stopped with SIGINT after DEADLINE_MIN minutes (default 40; the runner resumes).
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt trimtools" scripts/m5_trimtools.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_trimtools
DEADLINE_MIN=${DEADLINE_MIN:-40}
TASKS=(coding_01_my_sqrt_review coding_02_fizzbuzz_gen coding_03_compress_gen coding_05_scientific_lan coding_06_tax_rates_lan repoqa_05_retry_policy bugfix_01_moving_average refactor_02_rename_price)
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
timeout -s INT ${DEADLINE_MIN}m python -m harness.runner run --configs configs/sweep/sw_32k_trimtools.yaml \
    --tasks "${TASKS[@]}" --n 5 --ignore-task-trials --seed 5 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k_trimtools \
    && python -m analysis.figures --run $OUT && python -m analysis.claims --run $OUT || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
