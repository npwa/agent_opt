#!/bin/bash
# GPU session for plan items P2 (determinism), P6 (qwen3:8b smoke) and the first qwen3:8b screen (M5.4).
#
#   scripts/session_p2_p6.sh p2 survey p6 core      # whole session, in this order (about 2-2.5 h)
#   scripts/session_p2_p6.sh p2                     # any subset; every stage is resumable (re-run to continue)
#
# Stages
#   p2      determinism: 4 tasks x 6 repeats, temperature 0, system Ollama                       (about 20-25 min)
#   survey  start the two qwen3:8b servers once and record layers on GPU / KV size                (about 3 min)
#   p6      smoke: every task once on qwen3:8b (32k, KV q8_0), checks that tool calls work        (about 20-40 min)
#   core    qwen3:8b on the core suite, n = 3 (first screen of M5.4; only worth running if p6 looks sane) (about 1 h)
#
# Needs: Ollama running, nothing else on the GPU, `docker start searxng` for the research task (done below).
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
PROXY_PID=""

start_proxy() {  # $1 = output directory (the proxy writes to $1/proxy)
    mkdir -p "$1/proxy"
    AGENT_OPT_DATA_DIR="$1/proxy" python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > "$1/proxy/uvicorn.log" 2>&1 &
    PROXY_PID=$!
    sleep 3
}
stop_proxy() { [ -n "$PROXY_PID" ] && kill "$PROXY_PID" 2>/dev/null; PROXY_PID=""; }
trap stop_proxy EXIT

for stage in "$@"; do
    echo "=== $stage: $(date +%H:%M:%S)"
    case $stage in
    p2)
        OUT=results/v1/determinism
        start_proxy $OUT
        python -m harness.runner run --configs configs/determinism_t0.yaml \
            --tasks coding_05_scientific_lan coding_06_tax_rates_lan bugfix_01_moving_average repoqa_05_retry_policy \
            --n 6 --ignore-task-trials --seed 1 --out $OUT --scratch /tmp/agent_opt_scratch
        python -m analysis.determinism --run $OUT
        stop_proxy ;;
    survey)
        python -m harness.server survey --configs configs/sweep/sw_q3_8b_32k.yaml configs/sweep/sw_q3_8b_32k_kvq8.yaml \
            --out results/v1/server_survey_q3_8b ;;
    p6)
        OUT=results/v1/q3_8b_smoke
        docker start searxng > /dev/null 2>&1
        start_proxy $OUT
        python -m harness.runner run --configs configs/sweep/sw_q3_8b_32k_kvq8.yaml --n 1 --ignore-task-trials --seed 1 \
            --out $OUT --scratch /tmp/agent_opt_scratch
        python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT
        stop_proxy ;;
    core)
        OUT=results/v1/q3_8b_core_n3
        docker start searxng > /dev/null 2>&1
        start_proxy $OUT
        python -m harness.runner run --configs configs/sweep/sw_q3_8b_32k_kvq8.yaml --suite core --n 3 --ignore-task-trials --seed 1 \
            --out $OUT --scratch /tmp/agent_opt_scratch
        python -m harness.trace --run $OUT && python -m harness.decompose --run $OUT && python -m analysis.report --run $OUT
        stop_proxy ;;
    *) echo "unknown stage $stage (p2 survey p6 core)"; exit 2 ;;
    esac
done
echo "=== done: $(date +%H:%M:%S)"
