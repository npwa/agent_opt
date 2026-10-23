#include <stdio.h>
#include <ctype.h>

int main() {
    int c;
    int lines = 0;
    int words = 0;
    int bytes = 0;
    int maxlen = 0;
    int current_line_length = 0;
    int in_word = 0;

    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (current_line_length > maxlen) {
                maxlen = current_line_length;
            }
            current_line_length = 0;
            in_word = 0;
        } else {
            current_line_length++;
            if (isspace(c)) {
                if (in_word) {
                    words++;
                    in_word = 0;
                }
            } else {
                if (!in_word) {
                    in_word = 1;
                }
            }
        }
    }

    if (in_word) {
        words++;
        if (current_line_length > maxlen) {
            maxlen = current_line_length;
        }
    }

    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}