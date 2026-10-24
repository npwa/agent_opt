#include <stdio.h>

int main(void) {
    long lines = 0, words = 0, bytes = 0, maxlen = 0;
    int cur_line = 0;
    int in_word = 0;

    int c;
    while ((c = getchar()) != EOF) {
        bytes++;

        unsigned char uc = (unsigned char)c;

        if (uc == '\n') {
            lines++;
            if (cur_line > maxlen)
                maxlen = (long)cur_line;
            cur_line = 0;
            in_word = 0;
        } else {
            cur_line++;
            int is_ws = (uc == ' ' || uc == '\t' || uc == '\v' || uc == '\f' || uc == '\r');
            if (is_ws) {
                in_word = 0;
            } else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    if (cur_line > maxlen)
        maxlen = (long)cur_line;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);

    return 0;
}