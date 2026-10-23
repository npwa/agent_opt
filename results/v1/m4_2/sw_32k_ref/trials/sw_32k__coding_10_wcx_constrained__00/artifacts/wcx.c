#include <stdio.h>

static int is_white(int b)
{
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r';
}

int main(void)
{
    long lines = 0, words = 0, bytes = 0;
    int maxlen = 0;
    int cur_len = 0;
    int in_word = 0;
    int c;

    while ((c = getchar()) != EOF) {
        unsigned char uc = (unsigned char)c;
        bytes++;

        if (uc == '\n') {
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;
            in_word = 0;
        } else if (is_white(uc)) {
            if (in_word) {
                words++;
                in_word = 0;
            }
            cur_len++;
        } else {
            cur_len++;
            in_word = 1;
        }
    }

    /* Count last line without final newline */
    if (bytes > 0 && in_word) {
        lines++;
        words++;
        if (cur_len > maxlen)
            maxlen = cur_len;
    } else if (!in_word) {
        /* Last char was \n — no trailing line to add.
         * But \n already counted the current line above.
         * Need to handle: the word count for that last line needs counting */
    }

    printf("%ld %ld %ld %d\n", lines, words, bytes, maxlen);
    return 0;
}
