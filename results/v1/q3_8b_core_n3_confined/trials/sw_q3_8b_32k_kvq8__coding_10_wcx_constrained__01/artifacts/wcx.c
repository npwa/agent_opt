#include <stdio.h>
#include <ctype.h>

int main() {
    int c;
    int lines = 0, words = 0, bytes = 0;
    int maxlen = 0;
    int current_line_length = 0;
    int in_word = 0;

    while ((c = getc(stdin)) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (current_line_length > maxlen) {
                maxlen = current_line_length;
            }
            current_line_length = 0;
            in_word = 0;
        } else if (isspace(c)) {
            if (in_word) {
                words++;
                in_word = 0;
            }
            current_line_length += 1;
        } else {
            if (!in_word) {
                words++;
                in_word = 1;
            }
            current_line_length += 1;
        }
    }

    // Check if the last line has content and increment lines
    if (current_line_length > 0) {
        lines += 1;
        if (current_line_length > maxlen) {
            maxlen = current_line_length;
        }
    }

    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}