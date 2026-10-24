#include <stdio.h>

static int is_whitespace(unsigned c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
            c == '\f' || c == '\r');
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;

    int cur_len = 0;
    int in_word = 0;

    int ch;
    while ((ch = getchar()) != EOF) {
        unsigned char c = (unsigned char)ch;
        bytes++;

        if (c == '\n') {
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;
            in_word = 0;
        } else if (is_whitespace(c)) {
            cur_len++;
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            cur_len++;
            if (!in_word)
                in_word = 1;
        }
    }

    /* Un-terminated last line contributes one more to lines */
    if (bytes > 0 && cur_len > 0) {
        lines++;
        if (cur_len > maxlen)
            maxlen = cur_len;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
