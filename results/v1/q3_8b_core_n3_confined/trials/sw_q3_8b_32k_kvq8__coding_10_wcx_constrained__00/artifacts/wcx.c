#include <stdio.h>
#include <stdlib.h>

int main() {
    int lines = 0;
    int words = 0;
    size_t bytes = 0;
    size_t maxlen = 0;
    int inword = 0;
    char buf[65536];
    char *p = buf;
    size_t n = 0;
    int c;

    while ((c = fgetc(stdin)) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (n > maxlen) maxlen = n;
            n = 0;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            if (inword) {
                words++;
                inword = 0;
            }
            n = 0;
        } else {
            n++;
            inword = 1;
        }
        *p++ = c;
        if ((size_t)(p - buf) >= (size_t)sizeof(buf)) {
            fwrite(buf, 1, sizeof(buf), stdout);
            p = buf;
        }
    }
    if (n > maxlen) maxlen = n;
    words += inword;
    printf("%d %d %zu %zu\n", lines, words, bytes, maxlen);
    return 0;
}