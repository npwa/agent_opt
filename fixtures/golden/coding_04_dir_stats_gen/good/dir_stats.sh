#!/bin/bash
dir="${1:-.}"
if [ ! -d "$dir" ]; then
    echo "Error: Directory not found" >&2
    exit 1
fi
files=0; dirs=0; size=0
for e in "$dir"/* "$dir"/.[!.]*; do
    [ -e "$e" ] || continue
    if [ -f "$e" ]; then
        files=$((files + 1))
        size=$((size + $(stat -c %s "$e")))
    elif [ -d "$e" ]; then
        dirs=$((dirs + 1))
    fi
done
echo "Files: $files"
echo "Directories: $dirs"
echo "Total Size: $size bytes"
