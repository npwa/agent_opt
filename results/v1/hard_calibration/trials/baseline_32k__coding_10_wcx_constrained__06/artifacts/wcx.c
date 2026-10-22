#include <stdio.h>
#include <stddef.h>

static int is_whitespace(unsigned char b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long cur_lines = 0;
    long cur_maxlen = 0;
    int in_word = 0;

    for (;;) {
        int c = getchar();
        if (c == EOF) {
            /* final line without newline counts */
            if (cur_maxlen > maxlen)
                maxlen = cur_maxlen;
            break;
        }
        bytes++;
        cur_lines++;
        if (c == '\n') {
            lines++;
            if (cur_maxlen > maxlen)
                maxlen = cur_maxlen;
            cur_maxlen = 0;
            in_word = 0;
        } else {
            cur_maxlen++;
            unsigned char ub = (unsigned char)c;
            if (!is_whitespace(ub)) {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            } else {
                in_word = 0;
            }
        }
    }

    /* If no input at all, maxlen stays 0 which is correct */

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
