# bash coding exercise

Write a Bash script named `dir_stats.sh` that analyzes the contents of a given directory path.

**Requirements:**
1. Accept a single directory path as an argument ($1). Default to the current directory (`.`) if no argument is passed.
2. If the specified directory does not exist, print "Error: Directory not found" to stderr and exit with code 1.
3. Traverse the directory (non-recursively) and count:
   - Total number of regular files
   - Total number of subdirectories
   - Total size of all regular files in bytes
4. Output the results in the exact format:
   Files: <count>
   Directories: <count>
   Total Size: <bytes> bytes

Example Output:
Files: 12
Directories: 3
Total Size: 45021 bytes
