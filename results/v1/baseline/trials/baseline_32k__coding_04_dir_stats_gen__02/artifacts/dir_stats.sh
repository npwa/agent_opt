#!/usr/bin/env bash

DIR="${1:-.}"

if [ ! -d "$DIR" ]; then
  echo "Error: Directory not found" >&2
  exit 1
fi

file_count=$(find "$DIR" -maxdepth 1 -type f | wc -l)
dir_count=$(find "$DIR" -maxdepth 1 -type d | wc -l)
# Subtract 1 from dir_count to exclude the directory itself
dir_count=$((dir_count - 1))

total_size=0
while IFS= read -r -d '' file; do
  size=$(stat -c '%s' "$file" 2>/dev/null || echo 0)
  total_size=$((total_size + size))
done < <(find "$DIR" -maxdepth 1 -type f -print0)

echo "Files: $file_count"
echo "Directories: $dir_count"
echo "Total Size: $total_size bytes"
