#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    int lines = 0, words = 0, bytes = 0, maxlen = 0;
    int in_word = 0;
    size_t current_word_length = 0;
    size_t current_line_length = 0;
    int in_line = 1; // Whether we are in a line (not yet at a newline)

    ssize_t n;
    char buf[4096];

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (size_t i = 0; i < n; i++) {
            char c = buf[i];
            bytes += 1;

            if (c == '\n') {
                lines += 1;
                if (current_line_length > maxlen) {
                    maxlen = current_line_length;
                }
                current_line_length = 0;
                in_line = 0;
                in_word = 0;
                current_word_length = 0;
            } else if (isspace(c)) {
                if (in_word) {
                    words += current_word_length;
                    current_word_length = 0;
                    in_word = 0;
                }
                if (in_line) {
                    current_line_length += 1;
                }
            } else {
                if (in_line) {
                    current_line_length += 1;
                } else {
                    in_line = 1;
                }
                if (in_word) {
                    current_word_length += 1;
                } else {
                    in_word = 1;
                    current_word_length = 1;
                }
            }
        }
    }

    // Handle the last line if it doesn't end with a newline
    if (in_line) {
        if (current_line_length > maxlen) {
            maxlen = current_line_length;
        }
    }

    // Account for the last word if in_word is true
    if (in_word) {
        words += current_word_length;
    }

    // Print the result
    printf("%d %d %d %d\n", lines, words, bytes, maxlen);
    return 0;
}