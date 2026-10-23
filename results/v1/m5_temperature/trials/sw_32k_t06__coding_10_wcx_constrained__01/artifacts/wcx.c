#include <stdio.h>

int main(void)
{
    unsigned long lines = 0;
    unsigned long words = 0;
    unsigned long bytes = 0;
    unsigned long maxlen = 0;
    int in_word = 0;
    unsigned long cur_line = 0;
    int c;

    while ((c = getchar()) != EOF) {
        bytes++;

        if (c == '\n') {
            lines++;
            if (cur_line > maxlen)
                maxlen = cur_line;
            cur_line = 0;
            in_word = 0;
        } else {
            int is_ws = (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
            cur_line++;
            if (!is_ws) {
                if (!in_word) {
                    in_word = 1;
                    words++;
                }
            } else {
                in_word = 0;
            }
        }
    }

    if (cur_line > maxlen)
        maxlen = cur_line;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);

    return 0;
}
