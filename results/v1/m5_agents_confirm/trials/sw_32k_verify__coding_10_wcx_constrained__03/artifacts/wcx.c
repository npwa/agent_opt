#include <stdio.h>

/* Whitespace bytes for word counting: space, TAB, LF, VT, FF, CR */
static int is_ws(int ch) {
    return (ch == ' ' || ch == '\t' || ch == '\n' ||
            ch == '\v' || ch == '\f' || ch == '\r');
}

int main(void) {
    int c;
    long lines = 0, words = 0, bytes = 0;
    long maxlen = 0, curline = 0;
    int in_word = 0;

    while ((c = fgetc(stdin)) != EOF) {
        ++bytes;

        if (c == '\n') {
            ++lines;
            if (curline > maxlen)
                maxlen = curline;
            curline = 0;
            in_word = 0;
        } else {
            ++curline;
            if (is_ws(c))
                in_word = 0;
            else if (!in_word) {
                ++words;
                in_word = 1;
            }
        }
    }

    /* If input does not end with \n, the remaining chars are still part of
       the longest line (final text line without a trailing newline). */
    if (curline > maxlen)
        maxlen = curline;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
