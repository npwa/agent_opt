#!/bin/bash

dir="${1:-.}"

if [ ! -d "$dir" ]; then
    echo "Error: Directory not found" >&2
    exit 1
fi

files=0
dirs=0
total_size=0

for entry in "$dir"/*; do
    [ -e "$entry" ] || continue
    if [ -f "$entry" ]; then
        files=$((files + 1))
        size=$(stat -c %s "$entry" 2>/dev/null || stat -f %z "$entry" 2>/dev/null)
        total_size=$((total_size + size))
    elif [ -d "$entry" ]; then
        dirs=$((dirs + 1))
    fi
done

echo "Files: $files"
echo "Directories: $dirs"
echo "Total Size: $total_size bytes"
