#include <stdio.h>
#include <stdlib.h>

static int is_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    int maxlen = 0;
    int cur_line = 0;
    int in_word = 0;
    int ch;

    while ((ch = getchar()) != EOF) {
        unsigned char c = (unsigned char)ch;
        ++bytes;
        if (c == '\n') {
            ++lines;
            if (cur_line > maxlen)
                maxlen = cur_line;
            cur_line = 0;
            in_word = 0;
        } else if (is_ws(c)) {
            if (in_word)
                in_word = 0;
            ++cur_line;
        } else {
            if (!in_word) {
                in_word = 1;
                ++words;
            }
            ++cur_line;
        }
    }

    if (bytes > 0 && cur_line > maxlen)
        maxlen = cur_line;

    printf("%ld %ld %ld %d\n", lines, words, bytes, maxlen);

    return EXIT_SUCCESS;
}
