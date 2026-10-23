#include <stdio.h>

static int is_ws(unsigned char b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long num_bytes = 0;
    int maxlen = 0;
    int curline_len = 0;
    int in_word = 0;

    unsigned char buf[4096];

    while (1) {
        size_t n = fread(buf, 1, sizeof buf, stdin);
        if (n == 0)
            break;
        {
            size_t i;
            for (i = 0; i < n; i++) {
                unsigned char b = buf[i];
                num_bytes++;

                if (b == '\n') {
                    lines++;
                    if (curline_len > maxlen)
                        maxlen = curline_len;
                    curline_len = 0;
                    if (in_word) {
                        words++;
                        in_word = 0;
                    }
                } else if (is_ws(b)) {
                    if (in_word) {
                        words++;
                        in_word = 0;
                    }
                } else {
                    curline_len++;
                    in_word = 1;
                } else {
                    /* whitespace non-\n — still part of line for maxlen */
                }
            }
        }
    }

    if (curline_len > maxlen)
        maxlen = curline_len;

    if (in_word)
        words++;

    printf("%ld %ld %ld %d\n", lines, words, num_bytes, maxlen);
    return 0;
}
