#include <stdio.h>
#include <unistd.h>

#define BUF_SZ (1 << 12)

static inline int is_ws(unsigned char b)
{
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r';
}

int main(void)
{
    char buf[BUF_SZ];
    size_t n;

    int lines = 0, words = 0, bytes = 0, maxlen = 0;
    int cur_line_len = 0;
    int in_word = 0;

    while ((size_t)(n = read(STDIN_FILENO, buf, BUF_SZ)) > 0) {
        for (size_t i = 0; i < n; ++i) {
            unsigned char b = (unsigned char)buf[i];

            bytes++;

            if (b == '\n') {
                if (cur_line_len > maxlen)
                    maxlen = cur_line_len;
                cur_line_len = 0;
                lines++;
                in_word = 0;
            } else if (!is_ws(b)) {
                cur_line_len++;
                if (!in_word) {
                    in_word = 1;
                    words++;
                }
            } else {
                cur_line_len++;
                in_word = 0;
            }
        }
    }

    if (cur_line_len > maxlen)
        maxlen = cur_line_len;

    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}
