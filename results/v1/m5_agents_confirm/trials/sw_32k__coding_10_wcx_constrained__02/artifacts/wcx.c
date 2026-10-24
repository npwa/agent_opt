#include <stdio.h>

static int is_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

int main(void) {
    int c;
    long lines = 0, words = 0, bytes = 0;
    long currlen = 0, maxlen = 0;
    int in_word = 0;

    while ((c = getchar()) != EOF) {
        unsigned char uc = (unsigned char)c;
        int is_nl = uc == '\n';

        bytes++;

        if (is_nl) {
            lines++;
            if (currlen > maxlen)
                maxlen = currlen;
            currlen = 0;
            if (in_word) {
                words++;
                in_word = 0;
            }
            continue;
        }

        currlen++;

        if (is_space(uc)) {
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            in_word = 1;
        }
    }

    /* unterminated final line */
    if (in_word || currlen > 0) {
        lines ++;
        if (currlen > maxlen)
            maxlen = currlen;
        if (in_word)
            words++;
    } else if (bytes == 0 && lines == 0) {
        /* truly empty input: no content => zero for the unterminated line case */
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
