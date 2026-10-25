#!/bin/bash
# Warm re-run of the prompt replay after the perturbation fix (m5_promptreplay's warm columns are an artefact: the replay changed
# the shared TMPDIR text of the bash tool, which real trials never do). Two tasks, 2 repeats. About 25 min.
set -u
cd "$(dirname "$0")/.." || exit 1
. .venv/bin/activate
OUT=results/v1/m5_promptreplay_warm
REQ=results/v1/m5_specdecode/capture/proxy/requests
DROP4=task+todowrite+skill+webfetch
DROP7=$DROP4+list_mcp_resource_templates+list_mcp_resources+read_mcp_resource
for T in comms_12_timezone_slot repoqa_05_retry_policy; do
    echo "=== $T: $(date '+%F %H:%M:%S')"
    python -m harness.promptreplay run --requests $REQ/sw_32k__${T}__00 --variants default drop_tools=$DROP4 drop_tools=$DROP7 \
        --repeats 2 --out $OUT/$T
done
echo "=== done: $(date '+%F %H:%M:%S')"
