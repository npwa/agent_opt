#include <stdio.h>

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n'
        || c == '\v' || c == '\f'
        || c == '\r';
}

int main(void) {
    int ch;
    size_t bytes = 0, lines = 0, words = 0, maxlen = 0, curlen = 0;
    enum { OUT_W, IN_W } ws = OUT_W;

    while ((ch = getchar()) != EOF) {
        bytes++;
        if (ch == '\n') {
            lines++;
            if (maxlen < curlen)
                maxlen = curlen;
            curlen = 0;
            ws = OUT_W;
        } else {
            curlen++;
            if (ws == OUT_W && !is_ws(ch)) {
                words++;
                ws = IN_W;
            } else if (ws == IN_W && is_ws(ch)) {
                ws = OUT_W;
            }
        }
    }

    if (curlen > 0) {
        if (maxlen < curlen)
            maxlen = curlen;
    }

    printf("%zu %zu %zu %zu\n", lines, words, bytes, maxlen);
    return 0;
}
