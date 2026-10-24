#include <stdio.h>

/*
 * wcx: count lines, words, bytes and the longest line.
 */

int main(void)
{
    int c;
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long cur_len = 0;     /* current line length so far (without \n) */

    /* State: are we currently scanning the interior of a word? */
    int in_word = 0;

    while ((c = getchar()) != EOF) {
        bytes++;

        if (c == '\n') {
            /* End of line — record its length for maxlen. */
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;

            /* \n is whitespace: if we were mid-word, count it now. */
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
            /* Word separator, but NOT line-terminating. */
            if (in_word) {
                words++;
                in_word = 0;
            }
            cur_len++;
        } else if (c == '\r') {
            /* \r is whitespace for word-counting per spec, but an
               ordinary byte for maxlen. */
            if (in_word) {
                words++;
                in_word = 0;
            }
            cur_len++;
        } else {
            /* Inside a word. */
            if (!in_word) {
                words++;
                in_word = 1;
            }
            cur_len++;
        }
    }

    /* Finalize the last line (if input did not end with \n). */
    if (cur_len > maxlen)
        maxlen = cur_len;

    /* If we are still inside a word at EOF, count it. */
    if (in_word)
        words++;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
