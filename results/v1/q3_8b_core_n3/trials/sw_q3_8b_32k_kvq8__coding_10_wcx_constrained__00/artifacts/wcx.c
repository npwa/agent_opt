#include <stdio.h>
#include <ctype.h>

int main() {
    int lines = 0;
    int words = 0;
    int bytes = 0;
    int maxlen = 0;
    int inword = 0;
    int line_len = 0;
    int ch;

    while ((ch = getchar()) != EOF) {
        bytes++;
        if (ch == '\n') {
            lines++;
            if (line_len > maxlen) maxlen = line_len;
            line_len = 0;
        } else if (ispunct((unsigned char)ch)) {
            if (inword) {
                words++;
                inword = 0;
            }
            line_len = 0;
        } else {
            line_len++;
if (!inword) {
            words++;
            inword = 1;
        }
        }
    }

    if (line_len > maxlen) maxlen = line_len;
    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}