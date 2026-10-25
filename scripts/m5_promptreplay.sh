#!/bin/bash
# H1 prompt-trimming replay: recorded call sequences of four tasks (captured in m5_specdecode) replayed against Ollama with
# the prompt trimmed in three ways; first-call prefill cold and warm, 3 repeats, max_tokens 1. About 1 h.
#
#   systemd-inhibit --what=idle:sleep --why="agent_opt prompt replay" scripts/m5_promptreplay.sh
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_promptreplay
REQ=results/v1/m5_specdecode/capture/proxy/requests
DROP4=task+todowrite+skill+webfetch
DROP7=$DROP4+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource
mkdir -p $OUT
for T in repoqa_05_retry_policy bugfix_09_booking_slots comms_12_timezone_slot research_08e_free_will; do
    echo "=== $T: $(date '+%F %H:%M:%S')"
    python -m harness.promptreplay run --requests $REQ/sw_32k__${T}__00 --variants default drop_tools=$DROP4 drop_tools=$DROP7 strip_skills \
        --repeats 3 --out $OUT/$T
done
echo "=== done: $(date '+%F %H:%M:%S')"
