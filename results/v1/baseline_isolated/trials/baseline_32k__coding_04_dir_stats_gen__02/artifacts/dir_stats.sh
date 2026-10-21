#!/bin/bash

DIR="${1:-.}"
if [ ! -d "$DIR" ]; then
    echo "Error: Directory not found" >&2
    exit 1
fi

TOTAL_FILES=$(find "$DIR" -maxdepth 1 -mindepth 1 -type f | wc -l)
TOTAL_DIRS=$(find "$DIR" -maxdepth 1 -mindepth 1 -type d | wc -l)
TOTAL_SIZE=0
while IFS= read -r file; do
    [ -z "$file" ] && continue
    SIZE=$(stat -c%s "$file" 2>/dev/null || echo 0)
    TOTAL_SIZE=$((TOTAL_SIZE + SIZE))
done < <(find "$DIR" -maxdepth 1 -type f)

echo "Files: $TOTAL_FILES"
echo "Directories: $TOTAL_DIRS"
echo "Total Size: $TOTAL_SIZE bytes"
