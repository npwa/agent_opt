#include <stdio.h>

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    int c;
    unsigned long lines = 0, words = 0, bytes = 0;
    unsigned long maxlen = 0, cur = 0;
    int in_word_flag = 0;

    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (cur > maxlen)
                maxlen = cur;
            cur = 0;
            in_word_flag = 0;
        } else {
            cur++;
            if (!in_word_flag && !is_ws(c)) {
                words++;
                in_word_flag = 1;
            }
        }
    }

    if (cur > maxlen)
        maxlen = cur;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
