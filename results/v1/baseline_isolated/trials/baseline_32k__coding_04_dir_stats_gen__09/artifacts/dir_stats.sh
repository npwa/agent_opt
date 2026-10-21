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
  [ -e "$entry" ] || continue
  if [ -f "$entry" ]; then
    size=$(stat -c %s "$entry" 2>/dev/null || echo 0)
    total_size=$((total_size + size))
    file_count=$((file_count + 1))
  elif [ -d "$entry" ]; then
    dir_count=$((dir_count + 1))
  fi
done

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
