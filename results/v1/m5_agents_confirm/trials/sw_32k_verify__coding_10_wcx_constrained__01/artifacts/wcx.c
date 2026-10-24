/* wcx: count lines, words, bytes, and longest line length */

#include <stdio.h>

int main(void) {
    int c;
    long lines = 0, words = 0, bytes = 0, maxlen = 0, cur = 0;
    int in_word = 0;

    while ((c = getchar()) != EOF) {
        bytes++;

        if (c == '\n') {
            lines++;
            if (cur > maxlen)
                maxlen = cur;
            cur = 0;
        } else {
            cur++;

            int whitespace = 0;
            if      (c == ' ')    whitespace = 1;
            else if (c == '\t')   whitespace = 1;
            else if (c == '\r')   whitespace = 1;
            else if (c == '\n')   whitespace = 1;
            else if (c == '\v')   whitespace = 1;
            else if (c == '\f')   whitespace = 1;

            if (whitespace) {
                in_word = 0;
            } else if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }

    /* A last line without a trailing newline still counts for maxlen */
    if (cur > maxlen)
        maxlen = cur;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
