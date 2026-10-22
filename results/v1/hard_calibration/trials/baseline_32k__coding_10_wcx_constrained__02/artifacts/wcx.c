#include <stdio.h>

static int is_wspace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long line_count = 0, word_count = 0, byte_count = 0;
    size_t max_line_len = 0;
    int in_word = 0;
    unsigned cur_line = 0;
    int ch;

    while ((ch = getc(stdin)) != EOF) {
        unsigned char u = (unsigned char)ch;
        byte_count++;

        if (u == '\n') {
            if ((size_t)cur_line > max_line_len)
                max_line_len = (size_t)cur_line;
            cur_line = 0;
            in_word = 0;
            line_count++;
        } else {
            cur_line++;
            if (!is_wspace(u)) {
                if (!in_word) {
                    word_count++;
                    in_word = 1;
                }
            } else {
                in_word = 0;
            }
        }
    }

    /* trailing line without \n still counts */
    if ((int)cur_line > 0) {
        if ((size_t)cur_line > max_line_len)
            max_line_len = (size_t)cur_line;
        line_count++;
    }

    printf("%ld %ld %ld %zu\n", line_count, word_count, byte_count, max_line_len);
    return 0;
}
