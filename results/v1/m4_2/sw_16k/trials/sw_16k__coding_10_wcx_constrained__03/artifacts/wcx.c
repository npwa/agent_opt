#include <stdio.h>

int main(void)
{
    int c;
    int cur = 0, lines = 0, word_count = 0, total_bytes = 0, maxlen = 0;
    int was_space = 1, word_seen = 0;

    while ((c = fgetc(stdin)) != EOF) {
        total_bytes++;
        unsigned char uc = (unsigned char)c;

        /* classify whitespace */
        if (uc == ' ' || uc == '\t' || uc == '\n' || uc == '\v' || uc == '\f' || uc == '\r')
            was_space = 1;
        else
            was_space = 0;

        /* newline boundary → line ended */
        if (was_space == 0) {
            lines++;
            cur++;
        }

        /* new word starts where we were not-in-word and now see non-sp content */
        if (!word_seen && !was_space && cur > 0) {
            word_count++;
        }

        /* detect whitespace ending a word */
        if (was_space && word_seen) {
            word_count++;
            word_seen = 0;
        }

        if (was_space == 0) {
            word_seen = 1;
            cur++;
        } else if (was_space && was_newline) {
            cur = 0;
        } else if (was_space && !was_newline) {
            /* \r or other whitespace that is not newline — counts as content in maxlen */
            cur++;
        }

    }

    /* handle final line ending without a trailing \n */
    if (cur > 0 || word_seen) {
        lines++;
        if (cur > maxlen)
            maxlen = cur;
    }

    printf("%d %d %d %d\n", lines, word_count, total_bytes, maxlen);
    return 0;
}
