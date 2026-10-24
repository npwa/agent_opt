#include <stdio.h>

int main(void) {
    unsigned char buf[4096];
    size_t n;
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long cur_len = 0;
    int in_word = 0;

    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];
            bytes++;
            if (c == '\n') {
                lines++;
                if (cur_len > maxlen)
                    maxlen = cur_len;
                cur_len = 0;
                in_word = 0;
            } else {
                cur_len++;
                int is_space;
                switch (c) {
                case ' ': case '\t': case '\v': case '\f': case '\r':
                    is_space = 1;
                    break;
                default:
                    is_space = 0;
                    break;
                }
                if (!is_space) {
                    if (!in_word) {
                        words++;
                        in_word = 1;
                    }
                } else {
                    in_word = 0;
                }
            }
        }
    }
    /* Last line without trailing newline still counts for maxlen */
    if (cur_len > maxlen)
        maxlen = cur_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
