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

    if [ -f "$entry" ] && [ ! -L "$entry" ]; then
        file_count=$((file_count + 1))
        size=$(stat -c %s "$entry" 2>/dev/null || stat -f %z "$entry" 2>/dev/null)
        total_size=$((total_size + size))
    elif [ -d "$entry" ] && [ ! -L "$entry" ]; then
        dir_count=$((dir_count + 1))
    fi
done

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
