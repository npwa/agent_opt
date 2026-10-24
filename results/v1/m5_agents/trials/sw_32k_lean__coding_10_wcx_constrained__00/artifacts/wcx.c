#include <stdio.h>
#include <stdlib.h>  /* not needed - no malloc */

#define SZ 4096

static int is_whitespace(unsigned char c)
{
    switch (c) {
    case ' ': case '\t': case '\v': case '\f': case '\r':
        return 1;
    default:
        return 0;
    }
}

int main(void)
{
    unsigned char buf[SZ];
    size_t n;
    long lines = 0, words = 0, bytes = 0;
    long maxlen = 0, line_len = 0;
    int in_word = 0;

    while ((n = fread(buf, 1, SZ, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char b = buf[i];
            bytes++;

            if (b == '\n') {
                lines++;
                line_len = 0;
            } else {
                if (is_whitespace(b)) {
                    in_word = 0;
                } else if (!in_word) {
                    words++;
                    in_word = 1;
                }
                line_len++;
            }
        }
    }
    if (line_len > maxlen)
        maxlen = line_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
