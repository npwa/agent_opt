#include <stdio.h>
#include <limits.h>

static int is_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    int in_word = 0;
    long cur_line = 0;
    long max_len = 0;
    int c;

    while ((c = fgetc(stdin)) != EOF) {
        unsigned char uc = (unsigned char)c;

        if (uc == '\n') {
            lines++;
            if (cur_line > max_len)
                max_len = cur_line;
            cur_line = 0;
        } else {
            cur_line++;
            bytes++;
        }

        if (is_space(uc)) {
            in_word = 0;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }

    if (cur_line > max_len)
        max_len = cur_line;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, max_len);

    return 0;
}
