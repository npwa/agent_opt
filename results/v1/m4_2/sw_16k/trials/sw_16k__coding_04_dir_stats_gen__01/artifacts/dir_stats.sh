#!/usr/bin/env bash

# Usage: dir_stats.sh [directory_path]
# Analyzes the contents of a given directory (non-recursively)

dir="${1:-.}"

# Check if the directory exists
if [[ ! -d "$dir" ]]; then
    echo "Error: Directory not found" >&2
    exit 1
fi

file_count=0
dir_count=0
total_size=0

# Traverse directory non-recursively
for entry in "$dir"/*; do
    # Handle empty directories (glob doesn't expand)
    [[ -e "$entry" ]] || continue

    if [[ -f "$entry" ]]; then
        ((file_count++))
        # Get file size in bytes using stat
        file_size=$(stat -c '%s' "$entry" 2>/dev/null || stat -f '%z' "$entry" 2>/dev/null)
        total_size=$((total_size + file_size))
    elif [[ -d "$entry" ]]; then
        ((dir_count++))
    fi
done

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
