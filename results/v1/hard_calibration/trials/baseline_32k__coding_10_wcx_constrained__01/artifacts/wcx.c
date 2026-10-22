#include <stdio.h>
#include <unistd.h>

int main(void) {
    unsigned char buf[65536];
    long bytes = 0, words = 0, lines = 0;
    unsigned int maxlen = 0, cur_len = 0;
    int in_word = 0;

    while (1) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;

        bytes += n;

        for (ssize_t i = 0; i < n; i++) {
            unsigned char b = buf[i];

            if (b == '\n') {
                lines++;
                if (cur_len > maxlen)
                    maxlen = cur_len;
                cur_len = 0;
            } else {
                cur_len++;

                if (b == ' ' || b == '\t' || b == '\v'
                     || b == '\f' || b == '\r') {
                    in_word = 0;
                } else if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    if (cur_len > 0) {
        lines++;
        if (cur_len > maxlen)
            maxlen = cur_len;
    }

    printf("%ld %ld %ld %u\n", lines, words, bytes, maxlen);
    return 0;
}
