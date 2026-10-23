#include <stdio.h>
#include <ctype.h>

static int is_wc_whitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    int in_word = 0;
    long cur_line_len = 0;
    long max_line_len = 0;
    int c;

    while ((c = fgetc(stdin)) != EOF) {
        bytes++;

        if (c == '\n') {
            lines++;
            if (cur_line_len > max_line_len)
                max_line_len = cur_line_len;
            cur_line_len = 0;
            in_word = 0;
        } else {
            cur_line_len++;
            if (!is_wc_whitespace(c)) {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            } else {
                in_word = 0;
            }
        }
    }

    /* Handle last line without trailing newline */
    if (cur_line_len > max_line_len)
        max_line_len = cur_line_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, max_line_len);
    return 0;
}
