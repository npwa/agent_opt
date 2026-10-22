#include <stdio.h>

int main(void)
{
    int c;
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long curline = 0;
    enum { OUT, IN } state = OUT;

    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (curline > maxlen)
                maxlen = curline;
            curline = 0;
            state = OUT;
        } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
            if (state == IN)
                state = OUT;
            curline++;
        } else {
            if (state == OUT) {
                words++;
                state = IN;
            }
            curline++;
        }
    }

    /* If file doesn't end with newline, the last line still counts */
    if (curline > maxlen)
        maxlen = curline;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
