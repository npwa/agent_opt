#!/bin/bash

dir="${1:-.}"

if [ ! -d "$dir" ]; then
    echo "Error: Directory not found" >&2
    exit 1
fi

file_count=0
dir_count=0
total_size=0

for entry in "$dir"/*; do
    [ -e "$entry" ] || continue

    if [ -f "$entry" ]; then
        ((file_count++))
        size=$(stat -c %s "$entry" 2>/dev/null || echo 0)
        ((total_size += size))
    elif [ -d "$entry" ]; then
        ((dir_count++))
    fi
done

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
