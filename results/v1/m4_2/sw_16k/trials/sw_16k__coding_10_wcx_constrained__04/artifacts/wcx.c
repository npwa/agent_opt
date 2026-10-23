#include <stdio.h>

#define IS_W(b) ((b) == ' ' || (b) == '\t' || (b) == '\n' || (b) == '\v' || (b) == '\f' || (b) == '\r')

int main(void) {
    long lines = 0, words = 0;
    unsigned long bytes = 0, maxlen = 0;
    int in_word = 0;
    int c;
    unsigned long cur_line_len = 0;

    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (cur_line_len > maxlen)
                maxlen = cur_line_len;
            cur_line_len = 0;
        } else {
            cur_line_len++;
        }

        if (IS_W(c)) {
            in_word = 0;
        } else if (!in_word) {
            words++;
            in_word = 1;
        }
    }

    /* Count the final partial line and factor it into maxlen */
    lines += cur_line_len > 0;
    if (cur_line_len > maxlen)
        maxlen = cur_line_len;

    printf("%ld %ld %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
