#include <stdio.h>
#include <stdbool.h>

int main(void) {
    int ch;
    long lines = 0, words = 0, bytes = 0, maxlen = 0;
    long cur_len = 0;
    bool in_word = false;
    const char whitespace[] = " \t\n\v\f\r";

    while ((ch = fgetc(stdin)) != EOF) {
        bytes++;

        if (ch == '\n') {
            lines++;
            if (cur_len > maxlen)
                maxlen = cur_len;
            cur_len = 0;
            in_word = false;
        } else {
            cur_len++;
            bool is_ws = false;
            for (int i = 0; i < 6; i++) {
                if (ch == whitespace[i]) {
                    is_ws = true;
                    break;
                }
            }
            if (!is_ws) {
                if (!in_word) {
                    words++;
                    in_word = true;
                }
            } else {
                in_word = false;
            }
        }
    }

    /* last line: only consider it for maxlen if there was a byte */
    if (bytes > 0) {
        if (cur_len > maxlen)
            maxlen = cur_len;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
