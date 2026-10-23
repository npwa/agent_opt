#include <stdio.h>

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    int ch;
    long lines = 0, words = 0, bytes = 0;
    long maxlen = 0, curline = 0;
    int in_word = 0;

    while ((ch = getchar()) != EOF) {
        bytes++;
        if (ch == '\n') {
            if (curline > maxlen)
                maxlen = curline;
            lines++;
            curline = 0;
        } else {
            curline++;
        }

        if (is_ws(ch)) {
            in_word = 0;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }

    if (curline > maxlen)
        maxlen = curline;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
