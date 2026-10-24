#include <stdio.h>

int main(void) {
    int ch;
    long lines = 0;
    long words = 0;
    size_t maxlen = 0;
    size_t cur_len = 0;
    long bytes = 0;
    long in_word = 0;

    while ((ch = getchar()) != EOF) {
        if (ch == '\n') {
            /* newline terminates the current line */
            if (cur_len > maxlen)
                maxlen = cur_len;
            lines++;
            cur_len = 0;
            in_word = 0;       /* \n is whitespace; boundary between words */
        } else {
            const unsigned char c = (unsigned char)ch;

            /* \r is whitespace for word counting per the spec's six-byte list,
               but it is an "ordinary byte" for maxlen (does not terminate lines).*/
            int is_ws = (c == ' ' || c == '\t' || c == '\v' ||
                         c == '\f' || c == '\r');

            if (!is_ws && !in_word) {
                words++;
                in_word = 1;
            } else if (is_ws) {
                in_word = 0;
            }

            cur_len++;
        }

        bytes++;
    }

    /* final, unterminated line */
    if (cur_len > maxlen)
        maxlen = cur_len;

    printf("%ld %ld %ld %zu\n", lines, words, bytes, maxlen);
    return 0;
}
