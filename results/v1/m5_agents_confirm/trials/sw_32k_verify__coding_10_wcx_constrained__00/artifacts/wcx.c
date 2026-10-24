#include <stdio.h>

int main(void) {
    unsigned char buf[8192];
    long lines = 0;
    long words = 0;
    long bytes_count = 0;
    long maxlen = 0;
    long curline = 0;
    int in_word = 0;

    size_t n;
    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];

            if (c == '\n') {
                if (curline > maxlen)
                    maxlen = curline;
                curline = 0;
                lines++;
                in_word = 0;
            } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' ||
                       c == '\r') {
                in_word = 0;
                curline++;
            } else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
                curline++;
            }

            bytes_count++;
        }
    }

    /* Last line without a trailing \n still counts for maxlen */
    if (curline > maxlen)
        maxlen = curline;

    printf("%ld %ld %ld %ld\n", lines, words, bytes_count, maxlen);
    return 0;
}
