/* wcx: count lines, words, bytes, and longest line */
#include <stdio.h>

#define BUF (4096)

static inline int is_ws(int b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\v'
        || b == '\f' || b == '\r';
}

int main(void) {
    unsigned long lines = 0, words = 0, bytes = 0;
    unsigned long maxlen = 0, cur = 0;
    int in_word = 0;
    char buf[BUF];
    size_t n;

    while ((n = fread(buf, 1, BUF, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char b = (unsigned char)buf[i];
            bytes++;
            if (b == '\n') {
                lines++;
                if (cur > maxlen) maxlen = cur;
                cur = 0;
                in_word = 0;
            } else {
                cur++;
                if (!is_ws(b)) {
                    if (!in_word) { words++; in_word = 1; }
                } else {
                    in_word = 0;
                }
            }
        }
    }
    if (cur > maxlen) maxlen = cur;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
