#include <stdio.h>
#include <ctype.h>

int main(void) {
    int in_word = 0;
    long lines = 0, words = 0, bytes = 0;
    int cur_len = 0, max_len = 0;
    int c;

    while ((c = getchar()) != EOF) {
        bytes++;

        if (c == '\n') {
            if (cur_len > max_len)
                max_len = cur_len;
            lines++;
            cur_len = 0;
            in_word = 0;
        } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
            in_word = 0;
            cur_len++;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            cur_len++;
        }
    }

    if (bytes > 0 && cur_len > max_len)
        max_len = cur_len;

    printf("%ld %ld %ld %d\n", lines, words, bytes, max_len);
    return 0;
}
