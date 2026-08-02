#!/bin/bash
# Usage: send_email.sh [-s <subject>] <recipient> <body>
# Sandboxed: does not send real email. Appends to email_store.json in the
# same directory as this script, mirroring create_event.sh's mock pattern.

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
store="$script_dir/email_store.json"

usage() {
    echo "Usage: $0 [-s <subject>] <recipient> <body>" >&2
    exit 2
}

log() {
    local level=$1; shift
    local msg="$*"
    local ts
    ts=$(date '+%Y-%m-%d %H:%M:%S')
    case $level in
        1) sev="Error"  ;;
        2) sev="Warning";;
        3) sev="Info"   ;;
        4) sev="Debug"  ;;
        *) sev="Unknown";;
    esac
    echo "$ts  $sev  $msg" >&2
}

subject=""
while getopts ":s:h" opt; do
    case $opt in
        s) subject="$OPTARG" ;;
        h) usage ;;
        \?) log 1 "invalid option: -$OPTARG"; usage ;;
        :) log 1 "option -$OPTARG requires an argument"; usage ;;
    esac
done
shift $((OPTIND - 1))

if [[ $# -ne 2 ]]; then
    usage
fi

recipient="$1"
body="$2"

if [[ -z "$recipient" ]]; then
    log 1 "recipient must not be empty"
    usage
fi

[[ -f "$store" ]] || echo "[]" > "$store"

ts=$(date '+%Y-%m-%d %H:%M:%S')

python3 - "$store" "$recipient" "$subject" "$body" "$ts" << 'PYEOF'
import json, sys
store_path, recipient, subject, body, ts = sys.argv[1:6]
with open(store_path) as f:
    emails = json.load(f)
emails.append({
    "to": recipient,
    "subject": subject,
    "body": body,
    "sent_at": ts,
})
with open(store_path, "w") as f:
    json.dump(emails, f, indent=2)
print(f"{ts}  Info  email logged (sandboxed): to={recipient} subject={subject!r}")
PYEOF
