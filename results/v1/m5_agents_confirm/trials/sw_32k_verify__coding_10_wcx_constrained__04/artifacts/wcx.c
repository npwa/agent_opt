#include <stdio.h>

static int is_whitespace(int b) {
    return b == ' ' || b == '\t' || b == '\n' ||
           b == '\v' || b == '\f' || b == '\r';
}

int main(void) {
    int ch;
    long lines  = 0;
    long words  = 0;
    long bytes  = 0;
    long maxlen = 0;
    long line_len = 0;
    int in_word = 0;

    while ((ch = fgetc(stdin)) != EOF) {
        bytes++;
        if (ch == '\n') {
            lines++;
            if (line_len > maxlen)
                maxlen = line_len;
            line_len = 0;
            in_word  = 0;
        } else {
            line_len++;
            if (!is_whitespace(ch)) {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            } else {
                in_word = 0;
            }
        }
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
