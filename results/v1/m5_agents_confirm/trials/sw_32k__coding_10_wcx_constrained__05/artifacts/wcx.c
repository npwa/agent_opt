#include <stdio.h>

int main(void) {
    unsigned char buf[4096];
    int nbytes;

    long lines = 0, words = 0, bytes = 0, maxlen = 0;
    long curlen = 0;
    int in_word = 0;

    while ((nbytes = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (int i = 0; i < nbytes; i++) {
            unsigned char c = buf[i];
            bytes++;
            if (c == '\n') {
                lines++;
                if (curlen > maxlen)
                    maxlen = curlen;
                curlen = 0;
                in_word = 0;
            } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
                curlen++;
                if (!in_word)
                    in_word = 1;
            } else {
                curlen++;
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    if (curlen > maxlen)
        maxlen = curlen;

    fprintf(stdout, "%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
