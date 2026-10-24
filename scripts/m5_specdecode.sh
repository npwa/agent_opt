#!/bin/bash
# M5.3 speculative decoding. Stage "capture": one trial of 10 diverse tasks through the proxy with request capture (about 25 min).
# Stage "sweep" (replay of the captured calls against llama-server per speculation setting) is added by harness/specsweep.py.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt M5.3" scripts/m5_specdecode.sh capture
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
STAGE=${1:-capture}
OUT=results/v1/m5_specdecode
if [ "$STAGE" = capture ]; then
    mkdir -p $OUT/capture/proxy
    docker start searxng > /dev/null 2>&1
    AGENT_OPT_CAPTURE=all AGENT_OPT_DATA_DIR=$OUT/capture/proxy python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > $OUT/capture/proxy/uvicorn.log 2>&1 &
    PROXY=$!
    trap 'kill $PROXY 2>/dev/null' EXIT
    sleep 3
    echo "=== capture: $(date '+%F %H:%M:%S')"
    python -m harness.runner run --configs configs/sweep/sw_32k.yaml --n 1 --ignore-task-trials --seed 1 \
        --tasks coding_02_fizzbuzz_gen coding_03_compress_gen bugfix_09_booking_slots c_ringbuf_03_ringbuffer logtriage_04_pool_exhaustion \
                repoqa_05_retry_policy refactor_02_rename_price comms_12_timezone_slot research_08e_free_will longctx_11_harbor_report \
        --out $OUT/capture --scratch /tmp/agent_opt_scratch
    echo "runner exit code $?"
    echo "=== done: $(date '+%F %H:%M:%S')"
fi
