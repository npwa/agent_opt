#include <stdio.h>
#include <unistd.h>

int main(void) {
    unsigned long lines = 0;
    unsigned long words = 0;
    unsigned long bytes = 0;
    unsigned long maxlen = 0;

    int in_word = 0;
    unsigned long cur_line = 0;

    char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = (unsigned char)buf[i];
            bytes++;

            if (c == '\n') {
                if (in_word) {
                    words++;
                    in_word = 0;
                }
                if (cur_line > maxlen)
                    maxlen = cur_line;
                lines++;
                cur_line = 0;
            } else if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
                cur_line++;
                if (in_word) {
                    words++;
                    in_word = 0;
                }
            } else {
                cur_line++;
                in_word = 1;
            }
        }
    }

    if (cur_line > maxlen)
        maxlen = cur_line;
    if (in_word)
        words++;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
