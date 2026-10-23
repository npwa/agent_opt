#include <stdio.h>

static int is_ws(int ch) {
    unsigned char c = (unsigned char)ch;
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;

    int in_word = 0;
    long cur_len = 0;

    int ch;
    while ((ch = getchar()) != EOF) {
        unsigned char c = (unsigned char)ch;

        if (c == '\n') {
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;
            in_word = 0;
        } else {
            bytes++;

            /* \r does NOT start/end words, but counts toward line length */
            int ws = is_ws(c);
            if (!in_word && !ws) {
                words++;
                in_word = 1;
            } else if (is_ws(c)) {
                in_word = 0;
            }

            cur_len++;
        }
    }

    /* handle last line without trailing newline */
    if (cur_len > maxlen)
        maxlen = cur_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
