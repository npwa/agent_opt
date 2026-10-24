#include <stdio.h>

int main(void)
{
    int ch;
    long lines = 0;       /* number of \n bytes              */
    long words = 0;       /* maximal runs of non-whitespace  */
    long bytes = 0;       /* total bytes read                */
    long maxlen = 0;      /* length of longest line          */
    long linelen = 0;     /* bytes in current line           */
    int in_word = 0;      /* are we currently inside a word? */

    while ((ch = getc(stdin)) != EOF) {
        bytes++;

        if (ch == '\n') {
            lines++;
            if (in_word) {
                words++;
                in_word = 0;
            }
            if (linelen > maxlen)
                maxlen = linelen;
            linelen = 0;
        } else {
            /* Check if this byte is whitespace per the spec */
            int white = (ch == ' ') || (ch == '\t') || (ch == '\v')
                     || (ch == '\f') || (ch == '\r');

            if (!white && !in_word)
                in_word = 1;

            /* Count the word only when exiting it via non-newline whitespace */
            if (white && in_word) {
                words++;
                in_word = 0;
            }

            linelen++;
        }
    }

    /* If file ended with data on the last line (no trailing \n),
       that last word still needs to be counted, and maxlen updated. */
    if (in_word)
        words++;
    if (linelen > maxlen)
        maxlen = linelen;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
