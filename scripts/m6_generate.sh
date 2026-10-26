#!/bin/bash
# M6 step 1: generate research answers for the 20 citation-heavy prompts (suite m6), config sw_64k (32k overflows: see results/v1/m6_research_32k_aborted), n = 2 -> 40 trials,
# about 2.5 h. The events of each trial hold the retrieved pages (search results, fetched pages) that the citation verifier matches against.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt M6 generate" scripts/m6_generate.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m6_research
mkdir -p $OUT/proxy
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs configs/sweep/sw_64k.yaml --suite m6 --n 2 --ignore-task-trials --seed 4 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_64k || echo "report FAILED"
echo "=== done: $(date '+%F %H:%M:%S')"
