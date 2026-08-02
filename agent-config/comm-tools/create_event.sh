#!/bin/bash
# Usage: create_event.sh "<title>" "<start_iso8601>" "<end_iso8601>" "<comma_separated_attendees>"
# Appends the event to calendar_store.json in the same directory as this script.
# This is a local mock calendar, not a real calendar service.

usage() {
    echo "Usage: $0 <title> <start_iso8601> <end_iso8601> <attendees_csv>" >&2
    exit 2
}

if [[ $# -ne 4 ]]; then
    usage
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
store="$script_dir/calendar_store.json"

title="$1"
start="$2"
end="$3"
attendees="$4"

if [[ -z "$title" || -z "$start" || -z "$end" ]]; then
    echo "Error: title, start, and end must not be empty" >&2
    usage
fi

[[ -f "$store" ]] || echo "[]" > "$store"

event_id="evt_$(date +%s%N)"
ts=$(date '+%Y-%m-%d %H:%M:%S')

python3 - "$store" "$event_id" "$title" "$start" "$end" "$attendees" "$ts" << 'EOF'
import json, sys
store_path, event_id, title, start, end, attendees, ts = sys.argv[1:8]
with open(store_path) as f:
    events = json.load(f)
events.append({
    "id": event_id,
    "title": title,
    "start": start,
    "end": end,
    "attendees": [a.strip() for a in attendees.split(",") if a.strip()],
    "created_at": ts,
})
with open(store_path, "w") as f:
    json.dump(events, f, indent=2)
print(f"{ts}  Info  event created: {event_id} ({title}, {start} - {end})")
EOF
