#include <stdio.h>
#include <string.h>

#define BUFSIZE 65536

static int is_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    unsigned char buf[BUFSIZE];
    size_t lines = 0, words = 0, bytes = 0, maxlen = 0;
    size_t cur_line_len = 0;
    int last_was_nl = 1;
    size_t n;

    while ((n = fread(buf, 1, BUFSIZE, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];
            bytes++;

            if (c == '\n') {
                if (cur_line_len > maxlen)
                    maxlen = cur_line_len;
                lines++;
                cur_line_len = 0;
                last_was_nl = 1;
            } else {
                cur_line_len++;
                if (!is_ws(c)) {
                    if (last_was_nl)
                        words++;
                    last_was_nl = 0;
                } else {
                    last_was_nl = 1;
                }
            }
        }
    }

    /* Handle any remaining bytes without a trailing newline */
    if (cur_line_len > maxlen)
        maxlen = cur_line_len;

    printf("%zu %zu %zu %zu\n", lines, words, bytes, maxlen);
    return 0;
}
