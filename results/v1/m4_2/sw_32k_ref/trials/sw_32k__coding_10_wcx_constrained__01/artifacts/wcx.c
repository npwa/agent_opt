#include <stdio.h>

int main(void) {
    int ch;
    long lines = 0, words = 0, bytes = 0;
    long linelen = 0, maxlen = 0;
    int in_word = 0;

    while ((ch = getchar()) != EOF) {
        bytes++;
        if (ch == '\n') {
            if (linelen > maxlen)
                maxlen = linelen;
            lines++;
            linelen = 0;
            in_word = 0;
        } else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\v' || ch == '\f' || ch == '\r') {
            in_word = 0;
            linelen++;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            linelen++;
        }
    }

    if (bytes > 0) {
        if (linelen > maxlen)
            maxlen = linelen;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
