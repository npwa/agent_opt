#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>

static int is_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
           c == '\r';
}

int main(void) {
    long lines = 0, words = 0, bytes = 0;
    long max_line = 0, cur_line = 0;
    int in_word = 0;
    int c;

    while ((c = getchar()) != EOF) {
        unsigned char ch = (unsigned char)c;
        bytes++;

        if (ch == '\n') {
            if (cur_line > max_line)
                max_line = cur_line;
            lines++;
            cur_line = 0;
        } else {
            cur_line++;
        }

        if (is_ws(ch))
            in_word = 0;
        else if (!in_word) {
            words++;
            in_word = 1;
        }
    }

    if (cur_line > max_line)
        max_line = cur_line;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, max_line);
    return 0;
}
