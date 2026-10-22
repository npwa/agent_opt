#include <stdio.h>

int main(void) {
    unsigned long long lines = 0, words = 0, bytes = 0, maxlen = 0, cur = 0;
    int in_word = 0, c;
    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (cur > maxlen) maxlen = cur;
            cur = 0;
        } else {
            cur++;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == 0 || c == 0xA0) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    if (cur > maxlen) maxlen = cur;
    printf("%llu %llu %llu %llu\n", lines, words, bytes, maxlen);
    return 0;
}
