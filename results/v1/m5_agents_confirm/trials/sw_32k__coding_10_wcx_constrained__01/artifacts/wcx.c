#include <stdio.h>
#include <stdlib.h>

static int is_ws(int c) {
    /* exact 6 whitespace bytes: space, \t, \n, \v, \f, \r */
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r');
}

int main(void) {
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long maxlen = 0;
    long cur_len = 0;

    unsigned char buf[4096];
    size_t have = 0, pos = 0;

    enum { OUTSIDE, INSIDE } wstate = OUTSIDE;

    for (;;) {
        /* fetch a chunk and process it byte-by-byte */
        if (pos == have) {
            have = fread(buf, 1, sizeof(buf), stdin);
            pos = 0;
            if (have == 0) break;
        }

        unsigned char c = buf[pos];
        pos++;
        bytes++;
        cur_len++;

        if (!is_ws(c)) {
            /* inside a word */
            if (wstate == OUTSIDE) { words++; wstate = INSIDE; }
        } else {
            /* whitespace byte — but not \n for line counting */
            if (c == '\n') {
                lines++;
                if (cur_len > maxlen)
                    maxlen = cur_len;
                cur_len = 0;
            }
            if (wstate == INSIDE) { words++; wstate = OUTSIDE; }
        }
    }

    /* last line without trailing newline */
    if (bytes > 0) {
        lines++;
        if (cur_len > maxlen)
            maxlen = cur_len;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, maxlen);
    return 0;
}
