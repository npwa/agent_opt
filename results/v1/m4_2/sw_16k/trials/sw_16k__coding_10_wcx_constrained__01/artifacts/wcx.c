#include <stdio.h>

static inline int is_ws(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

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
        curline++;

        if (c == '\n') {
            long llen = curline - 1;
            if (llen > maxlen)
                maxlen = llen;
            lines++;
            curline = 0;
            state = OUT;
        } else if (is_ws(c)) {
            state = OUT;
        } else {
            if (state == OUT) {
                words++;
                state = IN;
            }
        }
    }

    if (curline > 0 && maxlen < curline - 1)
        maxlen = curline - 1;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);

    return 0;
}
