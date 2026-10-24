#include <stdio.h>

static inline int is_ws(int b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r';
}

int main(void) {
    unsigned long lines = 0, words = 0, bytes = 0;
    size_t maxlen = 0;
    int in_word = 0;
    size_t cur_line = 0;

    for (;;) {
        int c = getchar();
        if (c == EOF) {
            if (cur_line > maxlen)
                maxlen = cur_line;
            break;
        }
        bytes++;

        if (c == '\n') {
            lines++;
            if (cur_line > maxlen)
                maxlen = cur_line;
            cur_line = 0;
            in_word = 0;
        } else if (is_ws(c)) {
            /* space, \t, \v, \f, \r — end of word but keep tracking line */
            in_word = 0;
            cur_line++;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            cur_line++;
        }
    }

    /* empty input: lines=0, words=0, bytes=0, maxlen=0 */
    printf("%lu %lu %lu %zu\n", lines, words, bytes, maxlen);
    return 0;
}
