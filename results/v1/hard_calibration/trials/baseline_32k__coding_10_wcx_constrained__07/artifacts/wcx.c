#include <stdio.h>

static int is_ws(unsigned char b) {
    return (b == ' '  || b == '\t' || b == '\n' ||
            b == '\v' || b == '\f' || b == '\r');
}

int main(void) {
    unsigned long lines = 0, words = 0, bytes = 0;
    unsigned long maxlen = 0, cur_len = 0;
    int in_word = 0;

    for (int c; (c = getchar()) != EOF; ) {
        unsigned char b = (unsigned char)c;
        bytes++;

        if (b == '\n') {
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;
            in_word = 0;
        } else {
            cur_len++;
            if (!in_word && !is_ws(b)) {
                words++;
                in_word = 1;
            }
        }
    }

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
