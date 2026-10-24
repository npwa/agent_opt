#include <string.h>
#include <stdio.h>

void compress_string(const char *src, char *dst);

int main(void) {
    char buf[128];
    int all_pass = 1;

    /* Test case from the requirements */
    compress_string("aabcccccaaa", buf);
    if (strcmp(buf, "a2b1c5a3") != 0) { printf("Test 1 FAIL: got %s\n", buf); all_pass=0; } else printf("Test 1 PASS\n");

    /* No repeats */
    compress_string("abcdefg", buf);
    if (strcmp(buf, "a1b1c1d1e1f1g") != 0) { printf("Test 2 FAIL: got %s (expected a1b1c1d1e1f1g)\n", buf); all_pass=0; } else printf("Test 2 PASS\n");

    /* All same character */
    compress_string("aaaaa", buf);
    if (strcmp(buf, "a5") != 0) { printf("Test 3 FAIL: got %s\n", buf); all_pass=0; } else printf("Test 3 PASS\n");

    /* Empty string */
    compress_string("", buf);
    if (buf[0] != '\0') { printf("Test 4 FAIL: got '%s'\n", buf); all_pass=0; } else printf("Test 4 PASS\n");

    /* Two identical non-consecutive runs */
    compress_string("aabbaabb", buf);
    if (strcmp(buf, "a2b2a2b2") != 0) { printf("Test 5 FAIL: got %s\n", buf); all_pass=0; } else printf("Test 5 PASS\n");

    /* Multi-digit count: 15 'a's */
    compress_string("aaaaaaaaaaaaaaa", buf);  /* 15 a's */
    if (strcmp(buf, "a15") != 0) { printf("Test 6 FAIL: got %s\n", buf); all_pass=0; } else printf("Test 6 PASS\n");

    return all_pass ? 0 : 1;
}
