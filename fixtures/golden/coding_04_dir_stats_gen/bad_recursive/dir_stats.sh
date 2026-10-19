#!/bin/bash
dir="${1:-.}"
[ -d "$dir" ] || { echo "Error: Directory not found" >&2; exit 1; }
echo "Files: $(find "$dir" -type f | wc -l)"
echo "Directories: $(find "$dir" -mindepth 1 -type d | wc -l)"
echo "Total Size: $(find "$dir" -type f -printf '%s\n' | paste -sd+ | bc) bytes"
