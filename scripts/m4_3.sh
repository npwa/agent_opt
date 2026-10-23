#!/bin/bash
# M4.3: what share of the wall-clock is the GPU actually working, per task category?
# One Nsight Systems capture (python -m harness.nsys capture) = one trial under a private nsys-wrapped Ollama.
# Two captures per category (two different tasks where the category has two cheap ones, else the same task twice).
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt M4.3" scripts/m4_3.sh full     # about 2-2.5 h
#   scripts/m4_3.sh test                                                              # one capture, about 8 min
#   scripts/m4_3.sh report                                                            # only (re)build the aggregate
#
# Resumable: a capture whose directory already holds nsys_alignment.json is skipped. The multi-hundred-MB nsys files
# live in the scratch directory and are deleted after each capture; only the aligned summaries are kept.
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
MODE=${1:-full}
ROOT=results/v1/m4_3
SCRATCH=/tmp/agent_opt_nsys
CAPTURES=(
    "bugfix bugfix_01_moving_average 1" "bugfix bugfix_09_booking_slots 2"
    "code_gen coding_02_fizzbuzz_gen 1" "code_gen coding_03_compress_gen 2"
    "code_review coding_01_my_sqrt_review 1" "code_review coding_01_my_sqrt_review 2"
    "comms comms_07_confirm_send 1" "comms comms_12_timezone_slot 2"
    "log_triage logtriage_04_pool_exhaustion 1" "log_triage logtriage_04_pool_exhaustion 2"
    "long_context longctx_06_tidal_report 1" "long_context longctx_11_harbor_report 2"
    "reasoning coding_05_scientific_lan 1" "reasoning coding_06_tax_rates_lan 2"
    "refactor refactor_02_rename_price 1" "refactor refactor_02_rename_price 2"
    "repo_qa repoqa_05_retry_policy 1" "repo_qa repoqa_05_retry_policy 2"
    "research research_08e_free_will 1" "research research_08e_free_will 2"
)
[ "$MODE" = test ] && { ROOT=results/v1/m4_3_test; CAPTURES=("bugfix bugfix_01_moving_average 1"); }
say() { echo "=== $1: $(date '+%F %H:%M:%S')"; }

capture_one() {  # $1 category, $2 task, $3 rep
    local out="$ROOT/$1__$2__rep$3" scratch="$SCRATCH/$1__$2__rep$3"
    [ -f "$out/nsys_alignment.json" ] && { echo "skip $out (done)"; return 0; }
    for attempt in 1 2; do
        rm -rf "$out" "$scratch"
        say "capture $1 / $2 / rep $3 (attempt $attempt)"
        if python -m harness.nsys capture --task "$2" --out "$out" --scratch "$scratch" > "$out.log" 2>&1; then
            rm -rf "$scratch"; tail -n 4 "$out.log" | head -n 2; return 0
        fi
        echo "capture failed (attempt $attempt): $(tail -n 2 "$out.log" | tr '\n' ' ' | cut -c1-200)"
        mkdir -p "$ROOT/failed"; cp "$out.log" "$ROOT/failed/$1__$2__rep$3.attempt$attempt.log"
        # make sure nothing is left behind before the next attempt
        for p in $(ss -ltnp 2>/dev/null | grep -E ":(11435|11436) " | grep -o 'pid=[0-9]*' | cut -d= -f2); do kill "$p" 2>/dev/null; done
        sleep 10
    done
    rm -rf "$scratch"
    return 1
}

if [ "$MODE" != report ]; then
    mkdir -p "$ROOT"
    docker start searxng > /dev/null 2>&1
    for c in "${CAPTURES[@]}"; do
        set -- $c
        capture_one "$1" "$2" "$3" || echo "GIVING UP on $1 / $2 / rep $3"
    done
fi
say report
python -m analysis.nsys_summary --captures "$ROOT" --out "$ROOT" | head -n 20
say done
