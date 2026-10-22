#include <stdio.h>

static inline int is_ws(unsigned char b)
{
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' ||
           b == '\f' || b == '\r';
}

int main(void)
{
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;

    int in_word = 0;
    long cur_line_len = 0;

    while (1) {
        int ch = getchar();
        if (ch == EOF)
            break;

        unsigned char c = (unsigned char)ch;

        bytes++;

        if (c == '\n') {
            lines++;
            if (cur_line_len > maxlen)
                maxlen = cur_line_len;
            cur_line_len = 0;
            in_word = 0;
        } else {
            cur_line_len++;
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

    if (cur_line_len > maxlen)
        maxlen = cur_line_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);

    return 0;
}
