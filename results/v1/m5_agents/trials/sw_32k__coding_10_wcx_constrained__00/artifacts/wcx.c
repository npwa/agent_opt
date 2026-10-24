#include <stdio.h>

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    int lines = 0, words = 0;
    unsigned long bytes = 0;
    size_t max_len = 0, line_len = 0;
    int in_word = 0;
    int c;

    while ((c = fgetc(stdin)) != EOF) {
        unsigned char by = (unsigned char)c;
        bytes++;

        if (by == '\n') {
            lines++;
            if (line_len > max_len)
                max_len = line_len;
            in_word = 0;
            line_len = 0;
        } else {
            line_len++;
            if (!is_ws(c)) {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            } else {
                in_word = 0;
            }
        }
    }

    if (line_len > max_len)
        max_len = line_len;

    printf("%d %d %lu %zu\n", lines, words, bytes, max_len);
    return 0;
}
