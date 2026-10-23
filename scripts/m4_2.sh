#!/bin/bash
# M4.2: does a 16k context window hurt?  (plan, M4.2; unattended, about 7 h)
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt M4.2" scripts/m4_2.sh full      # the real thing
#   scripts/m4_2.sh smoke                                                              # 10 min pipeline check
#   scripts/m4_2.sh report                                                             # only (re)build the reports
#
# Stages (each resumable: re-run the same command to continue)
#   main16   sw_16k: all 18 tasks, n = 10; the three tasks whose context cannot fit in 16k (two long-context tasks and
#            research_08e, which needs 64k) n = 3                                              (about 5 h)
#   ref32    sw_32k on the same private server and confinement, n = 5 (n = 3 for the long and the two slowest tasks):
#            the reference that makes the comparison independent of the earlier, unconfined baseline  (about 2 h; it is
#            cut off at the deadline below, so the 16k results always come first)
#   report   trace / decompose / report / claims for both runs and three comparisons
#
# DEADLINE_H (default 7.4) bounds the whole script; the reference stage is stopped with SIGINT when it is reached.
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
MODE=${1:-full}
START=$(date +%s)
DEADLINE_H=${DEADLINE_H:-7.4}
LONG=(longctx_06_tidal_report longctx_11_harbor_report research_08e_free_will)
HEAVY=(c_ringbuf_03_ringbuffer coding_10_wcx_constrained)
if [ "$MODE" = smoke ]; then
    ROOT=results/v1/m4_2_smoke; TASKS=(--tasks coding_05_scientific_lan coding_02_fizzbuzz_gen); N16=1; N32=1; NFOR16=(); NFOR32=()
else
    ROOT=results/v1/m4_2; TASKS=(); N16=10; N32=5
    NFOR16=(--n-for "${LONG[@]/%/=3}"); NFOR32=(--n-for "${LONG[@]/%/=3}" "${HEAVY[@]/%/=3}")
fi
PROXY_PID=""
start_proxy() {
    mkdir -p "$1/proxy"
    AGENT_OPT_DATA_DIR="$1/proxy" python -m uvicorn harness.proxy:app --host 127.0.0.1 --port 11435 > "$1/proxy/uvicorn.log" 2>&1 &
    PROXY_PID=$!
    sleep 3
}
stop_proxy() { [ -n "$PROXY_PID" ] && kill "$PROXY_PID" 2>/dev/null; PROXY_PID=""; }
trap stop_proxy EXIT
say() { echo "=== $1: $(date '+%F %H:%M:%S')"; }

run_stage() {  # $1 = config, $2 = out dir, $3 = n, rest = extra runner args; honours the deadline
    local cfg=$1 out=$2 n=$3; shift 3
    local left=$(python3 -c "print(max(60, int($DEADLINE_H*3600 - ($(date +%s) - $START))))")
    start_proxy "$out"
    timeout --signal=INT --kill-after=300 "$left" python -m harness.runner run --configs "$cfg" "${TASKS[@]}" --n "$n" \
        --ignore-task-trials --seed 1 --out "$out" --scratch /tmp/agent_opt_scratch "$@"
    echo "runner exit code $?"
    stop_proxy
    python -m harness.runner status --out "$out" --by-task
}

build_reports() {  # $1 = out dir
    python -m harness.trace --run "$1" && python -m harness.decompose --run "$1" && python -m analysis.report --run "$1" \
        && python -m analysis.figures --run "$1" && python -m analysis.claims --run "$1" \
        || echo "report for $1 FAILED"
}

stages=(main16 ref32 report)
[ "$MODE" = report ] && stages=(report)
docker start searxng > /dev/null 2>&1
for stage in "${stages[@]}"; do
    say "$stage"
    case $stage in
    main16) run_stage configs/sweep/sw_16k.yaml "$ROOT/sw_16k" $N16 "${NFOR16[@]}" ;;
    ref32)  run_stage configs/sweep/sw_32k.yaml "$ROOT/sw_32k_ref" $N32 "${NFOR32[@]}" ;;
    report)
        for d in sw_16k sw_32k_ref; do [ -d "$ROOT/$d" ] && build_reports "$ROOT/$d"; done
        [ -f "$ROOT/sw_16k/summary.json" ] && [ -f "$ROOT/sw_32k_ref/summary.json" ] && \
            python -m analysis.compare_runs --a "$ROOT/sw_32k_ref" --b "$ROOT/sw_16k" --out "$ROOT/COMPARISON_16k_vs_32k_ref.md"
        [ -f "$ROOT/sw_16k/summary.json" ] && \
            python -m analysis.compare_runs --a results/v1/baseline_isolated --b "$ROOT/sw_16k" --out "$ROOT/COMPARISON_16k_vs_isolated_baseline.md"
        [ -f "$ROOT/sw_32k_ref/summary.json" ] && \
            python -m analysis.compare_runs --a results/v1/baseline_isolated --b "$ROOT/sw_32k_ref" --out "$ROOT/COMPARISON_32k_ref_vs_isolated_baseline.md"
        ;;
    esac
done
say done
