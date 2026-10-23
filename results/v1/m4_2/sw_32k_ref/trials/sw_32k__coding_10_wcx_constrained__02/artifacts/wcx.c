#include <stdio.h>
#include <stdlib.h>

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long lines = 0, words = 0, bytes = 0;
    long maxlen = 0;
    long cur_line = 0;
    int in_word = 0;
    char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        for (size_t i = 0; i < n; ++i) {
            unsigned char ch = (unsigned char)buf[i];
            bytes++;
            if (is_ws(ch)) {
                if (in_word) {
                    words++;
                    in_word = 0;
                }
                if (ch == '\n') {
                    lines++;
                    if (cur_line > maxlen)
                        maxlen = cur_line;
                    cur_line = 0;
                } else {
                    cur_line++;
                }
            } else {
                cur_line++;
                in_word = 1;
            }
        }
    }

    if (bytes == 0) {
        maxlen = 0;
    } else if (in_word || cur_line > 0) {
        lines++;
        if (cur_line > maxlen)
            maxlen = cur_line;
        if (in_word) words++;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return EXIT_SUCCESS;
}
