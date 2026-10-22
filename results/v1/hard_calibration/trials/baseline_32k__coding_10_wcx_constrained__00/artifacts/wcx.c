#include <stdio.h>
#include <stdlib.h>

#define IS_WS(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || \
                  (c) == '\v' || (c) == '\f' || (c) == '\r')

int main(void)
{
    int ch;
    long lines = 0, words = 0, bytes = 0;
    long maxlen = 0;
    long linelen = 0;
    int in_word = 0;

    while ((ch = getc(stdin)) != EOF) {
        unsigned char c = (unsigned char)ch;
        bytes++;

        if (IS_WS(c)) {
            if (c == '\n') {
                lines++;
                if (linelen > maxlen)
                    maxlen = linelen;
                linelen = 0;
            }
            in_word = 0;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            linelen++;
        }
    }

    /* A final segment without trailing \n contributes to maxlen */
    if (linelen > maxlen && bytes > 0)
        maxlen = linelen;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return EXIT_SUCCESS;
}
