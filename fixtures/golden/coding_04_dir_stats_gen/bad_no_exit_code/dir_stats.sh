#!/bin/bash
dir="${1:-.}"
if [ ! -d "$dir" ]; then
    echo "Error: Directory not found"
    exit 0
fi
echo "Files: $(find "$dir" -maxdepth 1 -type f | wc -l)"
echo "Directories: $(find "$dir" -mindepth 1 -maxdepth 1 -type d | wc -l)"
echo "Total Size: 0 bytes"
