#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    char current_char = src[0];
    int current_count = 1;
    char *dest_ptr = dest;

    for (int i = 1; src[i] != '\0'; i++) {
        if (src[i] == current_char) {
            current_count++;
        } else {
            *dest_ptr++ = current_char;
            if (current_count > 9) {
                *dest_ptr++ = '0' + (current_count / 10);
                *dest_ptr++ = '0' + (current_count % 10);
            } else {
                *dest_ptr++ = '0' + current_count;
            }
            current_char = src[i];
            current_count = 1;
        }
    }
    *dest_ptr++ = current_char;
    if (current_count > 9) {
        *dest_ptr++ = '0' + (current_count / 10);
        *dest_ptr++ = '0' + (current_count % 10);
    } else {
        *dest_ptr++ = '0' + current_count;
    }
    *dest_ptr = '\0';
}