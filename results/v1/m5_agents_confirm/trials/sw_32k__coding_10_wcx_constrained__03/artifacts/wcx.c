#include <stdio.h>

int main(void) {
    unsigned char buf[4096];
    size_t n;
    long lines = 0, words = 0, bytes = 0, maxlen = 0;
    long cur = 0;
    int after_ws = 1;
    int last_nl = 0;

    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        for (size_t i = 0; i < n; ++i) {
            unsigned char b = buf[i];
            bytes++;
            if (b == '\n') {
                lines++;
                if (cur > maxlen)
                    maxlen = cur;
                cur = 0;
                after_ws = 1;
                last_nl = 1;
            } else if (b == ' ' || b == '\t' || b == '\v' || b == '\f' || b == '\r') {
                if (after_ws)
                    words++;
                after_ws = 1;
                cur++;
            } else {
                if (after_ws)
                    words++;
                after_ws = 0;
                cur++;
            }
        }
    }

    if (bytes > 0 && !last_nl) {
        lines++;
        if (cur > maxlen)
            maxlen = cur;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
