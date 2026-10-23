#include <stdio.h>

int main(void) {
    int c;
    long lines = 0, words = 0, bytes = 0, maxlen = 0;
    long curline = 0;
    int in_word = 0;

    while ((c = fgetc(stdin)) != EOF) {
        bytes++;

        if (c == '\n') {
            lines++;
            if (curline > maxlen)
                maxlen = curline;
            curline = 0;
            in_word = 0;
        } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
            /* Note: \r is counted in maxlen but treated as whitespace for word splitting */
            curline++;
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            curline++;
            if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }

    /* Handle last line without trailing newline */
    if (bytes > 0) {
        if (curline > maxlen)
            maxlen = curline;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
