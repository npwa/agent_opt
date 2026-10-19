# compress_string coding exercise

**Requirements:**

Write a C function `void compress_string(const char *src, char *dest)` that performs basic run-length compression on an input string. 

Requirements:
1. Replace consecutive repeated characters with the character followed by the count (e.g., "aabccc" -> "a2b1c3").
2. Assume the input contains only alphabetical characters (a-z, A-Z) and is null-terminated.
3. Assume `dest` has sufficient allocated space to store the result.
4. Output the null-terminated compressed string into `dest`.

Example:
Input: "aabcccccaaa"
Output: "a2b1c5a3"
