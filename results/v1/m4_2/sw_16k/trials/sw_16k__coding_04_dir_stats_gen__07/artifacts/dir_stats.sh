#!/bin/bash

dir="${1:-.}"

if [ ! -d "$dir" ]; then
  echo "Error: Directory not found" >&2
  exit 1
fi

files=0
total_size=0

for entry in "$dir"/*; do
  [ -e "$entry" ] || continue
  if [ -f "$entry" ]; then
    size=$(stat -c%s "$entry" 2>/dev/null || echo 0)
    total_size=$((total_size + size))
    files=$((files + 1))
  elif [ -d "$entry" ]; then
    dirs=$((dirs + 1))
  fi
done

echo "Files: $files"
echo "Directories: ${dirs:-0}"
echo "Total Size: $total_size bytes"
