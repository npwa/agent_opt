#include <stdio.h>

#define BUFSZ 4096

int main(void) {
    char buf[BUFSZ];
    unsigned long lines = 0, words = 0, bytes = 0, maxlen = 0;
    size_t inbuf = 0, nb = 0;
    unsigned long curline = 0;
    int inword = 0;

    while (1) {
        if (inbuf >= nb) {
            inbuf = 0;
            nb = fread(buf, 1, BUFSZ, stdin);
            if (nb == 0)
                break;
        }

        unsigned char c = (unsigned char)buf[inbuf++];
        bytes++;

        if (c == '\n') {
            lines++;
            if (curline > maxlen)
                maxlen = curline;
            curline = 0;
        } else {
            curline++;
        }

        int is_ws;
        switch (c) {
            case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
                is_ws = 1;
                break;
            default:
                is_ws = 0;
                break;
        }

        if (is_ws)
            inword = 0;
        else if (!inword) {
            words++;
            inword = 1;
        }
    }

    /* Last line without trailing newline still contributes its length */
    if (curline > maxlen)
        maxlen = curline;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
