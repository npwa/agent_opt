#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main() {
    FILE *fp = stdin;
    int ch;
    int lines = 0, words = 0, bytes = 0, maxlen = 0;
    int current_line_length = 0;
    int in_word = 0;

    while ((ch = fgetc(fp)) != EOF) {
        bytes++;
        if (ch == '\n') {
            lines++;
            if (current_line_length > maxlen) {
                maxlen = current_line_length;
            }
            current_line_length = 0;
            in_word = 0;
        } else if (isspace(ch)) {
            if (in_word) {
                in_word = 0;
            }
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            current_line_length++;
        }
    }

    // Check the last line if it doesn't end with newline
    if (current_line_length > maxlen) {
        maxlen = current_line_length;
    }

    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}