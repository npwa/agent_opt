#include <string.h>
#include <stdio.h>

/**
 * Run-length compresses an input string into dest.
 *
 * Consecutive repeated characters are replaced with the character
 * followed by its count (e.g., "aabccc" -> "a2b1c3").
 *
 * @param src  Input string, null-terminated, consisting of a-z / A-Z
 * @param dest Destination buffer (caller must ensure it is large enough)
 */
void compress_string(const char *src, char *dest)
{
    if (!src || !dest) {
        *dest = '\0';
        return;
    }

    const char *p = src;     /* walks through src            */
    char *d = dest;          /* writes into dest             */
    unsigned int len = strlen(src);

    /* Handle empty input trivially */
    if (len == 0) {
        *dest = '\0';
        return;
    }

    while (*p) {
        char current = *p;
        unsigned int cnt = 0;

        /* Count how many consecutive copies of the same character follow, including this one */
        while (*p && *p == current) {
            ++cnt;
            ++p;
        }

        /* Store original character (the first in the run) */
        *d++ = current;

        /* Convert cnt to decimal string digits */
        if (cnt >= 100) {
            *d++ = '0' + cnt / 100;
            *d++ = '0' + (cnt / 10) % 10;
            *d++ = '0' + cnt % 10;
        } else if (cnt >= 10) {
            *d++ = '0' + cnt / 10;
            *d++ = '0' + cnt % 10;
        } else {
            *d++ = '0' + cnt;
        }
    }

    *d = '\0';   /* null-terminate */
}

/* ---- simple self-test (compile with -o compress_test) ---- */
#ifdef COMPRESS_TEST
#include <string.h>

static void check(const char *name, const char *src, const char *expected)
{
    char buf[256];
    compress_string(src, buf);
    int pass = strlen(buf) == strlen(expected) && strcmp(buf, expected) == 0;
    printf("%s: %s  (got \"%s\", expected \"%s\")\n",
           name, pass ? "PASS" : "FAIL", buf, expected);
}

int main(void)
{
    check("example",        "aabcccccaaa",  "a2b1c5a3");
    check("single_char",   "a",            "a1");
    check("no_duplicates", "abc",          "a1b1c1");
    check("empty",         "",             "");
    check("all_same",      "zzzzz",        "z5");
    check("two_runs",      "aaabbb",       "a3b3");
    return 0;
}
#endif
