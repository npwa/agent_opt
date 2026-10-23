#include <stdio.h>
#include <limits.h>
#include <stdint.h>

int main(void) {
    int ch;
    uintmax_t lines = 0, words = 0, bytes = 0, maxlen = 0;
    uintmax_t curline = 0;
    int in_word = 0;
    int has_line_content = 0;

    while ((ch = getchar()) != EOF) {
        bytes++;

        if (ch == '\n') {
            lines++;
            if (curline > maxlen) {
                maxlen = curline;
            }
            curline = 0;
        } else {
            curline++;
        }

        int is_ws;
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\v' || ch == '\f' || ch == '\r') {
            is_ws = 1;
        } else {
            is_ws = 0;
        }

        if (!is_ws) {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            has_line_content = 1;
        } else {
            if (in_word && ch == '\n') {
                ; /* word break already counted above */
            }
            if (ch == '\n' || ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r') {
                in_word = 0;
            }
        }
    }

    if (has_line_content && curline > maxlen) {
        maxlen = curline;
    }
    if (!lines && has_line_content) {
        lines++;
    }

    printf("%ju %ju %ju %ju\n", lines, words, bytes, maxlen);
    return 0;
}
