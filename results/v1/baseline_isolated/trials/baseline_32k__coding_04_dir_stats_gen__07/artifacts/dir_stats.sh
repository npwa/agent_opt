#!/bin/bash

DIR="${1:-.}"

if [ ! -d "$DIR" ]; then
    echo "Error: Directory not found" >&2
    exit 1
fi

file_count=0
dir_count=0
total_size=0

for entry in "$DIR"/*; do
    if [[ -f "$entry" ]]; then
        ((file_count++))
        size=$(stat -c %s "$entry")
        total_size=$((total_size + size))
    elif [[ -d "$entry" ]]; then
        ((dir_count++))
    fi
done

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
