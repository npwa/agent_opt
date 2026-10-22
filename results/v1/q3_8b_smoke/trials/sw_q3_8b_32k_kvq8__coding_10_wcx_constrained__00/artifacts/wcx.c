#include <stdio.h>
#include <stdlib.h>

int main() {
    int lines = 0;
    int words = 0;
    long bytes = 0;
    int maxlen = 0;
    int state = 0; // 0 = word, 1 = whitespace
    int line_len = 0;

    int c;
    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            line_len = 0;
        } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
            if (state == 0) {
                words++;
                state = 1;
            }
        } else {
            if (state == 1) {
                words++;
                state = 0;
            }
            line_len++;
        }
        if (line_len > maxlen) {
            maxlen = line_len;
        }
    }

    if (state == 0 && line_len > 0) {
        words++;
    }

    printf("%d %d %ld %d\n", lines, words, bytes, maxlen);
    return 0;
}