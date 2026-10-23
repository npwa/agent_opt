#include <stdio.h>
#include <limits.h>

static int is_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    unsigned long lines = 0;
    unsigned long words = 0;
    unsigned long bytes = 0;
    unsigned long maxlen = 0;
    unsigned long curline = 0;
    int in_word = 0;
    unsigned char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];
            bytes++;
            if (c == '\n') {
                lines++;
                if (curline > maxlen)
                    maxlen = curline;
                curline = 0;
            } else {
                curline++;
            }
            if (is_ws(c)) {
                in_word = 0;
            } else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    if (curline > maxlen)
        maxlen = curline;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
