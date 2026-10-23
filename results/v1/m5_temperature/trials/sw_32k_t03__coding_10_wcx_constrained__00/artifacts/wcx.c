#include <stdio.h>

int main(void)
{
    unsigned char buf[4096];
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long curline = 0;
    int in_word = 0;

    while (1) {
        size_t n = fread(buf, 1, sizeof buf, stdin);
        if (n == 0)
            break;
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];
            bytes++;
            if (c == '\n') {
                lines++;
                if (curline > maxlen)
                    maxlen = curline;
                curline = 0;
                in_word = 0;
            } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
                in_word = 0;
            } else {
                curline++;
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    /* last line without trailing newline */
    if (curline > maxlen)
        maxlen = curline;
    if (in_word) {
        words++;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
