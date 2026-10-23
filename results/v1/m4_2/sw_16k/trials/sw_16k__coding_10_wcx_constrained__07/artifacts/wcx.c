#include <stdio.h>

static int is_ws(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
    int c;
    long lines = 0, words = 0, bytes = 0;
    int maxlen = 0, curline = 0;
    int inword = 0;

    while ((c = fgetc(stdin)) != EOF) {
        unsigned char ch = (unsigned char)c;
        bytes++;

        if (ch == '\n') {
            lines++;
            if (curline > maxlen) maxlen = curline;
            curline = 0;
            inword = 0;
        } else {
            curline++;
            if (!is_ws(ch)) {
                if (!inword) {
                    words++;
                    inword = 1;
                }
            } else {
                inword = 0;
            }
        }
    }

    if (bytes > 0 && curline > maxlen) maxlen = curline;

    printf("%ld %ld %ld %d\n", lines, words, bytes, maxlen);
    return 0;
}
