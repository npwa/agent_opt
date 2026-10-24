#include <stdio.h>
#include <stddef.h>

int main(void)
{
    unsigned long lines = 0;
    unsigned long words = 0;
    unsigned long nbytes = 0;
    unsigned long maxlen = 0;

    int in_word = 0;
    unsigned long cur_line = 0;

    char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];
            int is_ws;

            nbytes++;

            if (c == '\n') {
                if (cur_line > maxlen)
                    maxlen = cur_line;
                cur_line = 0;
                lines++;
            } else {
                cur_line++;
            }

            switch (c) {
            case ' ': case '\t': case '\n':
            case '\v': case '\f': case '\r':
                is_ws = 1;
                break;
            default:
                is_ws = 0;
                break;
            }

            if (is_ws)
                in_word = 0;
            else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    if (cur_line > maxlen)
        maxlen = cur_line;

    printf("%lu %lu %lu %lu\n", lines, words, nbytes, maxlen);
    return 0;
}
