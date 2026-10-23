#!/bin/bash
# M5 H8: does a lower sampling temperature raise the pass rate and the consistency of repeated trials?
# Three configurations on ONE private server block (their trials are interleaved in time): the control sw_32k (temperature 1.0,
# the model default), sw_32k_t03 and sw_32k_t06. Core suite, n = 3 -> 126 trials, about 3.3 h.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt H8" scripts/m5_temperature.sh full
#   scripts/m5_temperature.sh smoke       # 3 trials, 3 min: does the temperature reach the server?
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
MODE=${1:-full}
if [ "$MODE" = smoke ]; then OUT=results/v1/m5_temperature_smoke; ARGS=(--tasks coding_05_scientific_lan --n 1); else OUT=results/v1/m5_temperature; ARGS=(--suite core --n 3); fi
CFGS=(configs/sweep/sw_32k.yaml configs/sweep/sw_32k_t03.yaml configs/sweep/sw_32k_t06.yaml)
mkdir -p $OUT/proxy
docker start searxng > /dev/null 2>&1
AGENT_OPT_DATA_DIR=$OUT/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/proxy/uvicorn.log 2>&1 &
PROXY=$!
trap 'kill $PROXY 2>/dev/null' EXIT
sleep 3
echo "=== run: $(date '+%F %H:%M:%S')"
python -m harness.runner run --configs "${CFGS[@]}" "${ARGS[@]}" --ignore-task-trials --seed 1 --out $OUT --scratch /tmp/agent_opt_scratch
echo "runner exit code $?"
kill $PROXY 2>/dev/null
echo "=== report: $(date '+%F %H:%M:%S')"
python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT --baseline sw_32k \
    && python -m analysis.figures --run $OUT && python -m analysis.claims --run $OUT && python -m analysis.consistency --run $OUT || echo "report FAILED"
python3 - <<PYEOF
import json,collections
reqs=collections.Counter()
for l in open("$OUT/proxy/metrics.jsonl"):
    r=json.loads(l); reqs[(r["trial_id"].split("__")[0], r["req"].get("temperature"))]+=1
print("temperature seen by the proxy per configuration (calls):", dict(reqs))
PYEOF
echo "=== done: $(date '+%F %H:%M:%S')"
