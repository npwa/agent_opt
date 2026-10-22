#include <stdio.h>
#include <stdlib.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *current = src;
    char prev_char = *current;
    int count = 1;
    char *dest_ptr = dest;

    while (*current) {
        current++;
        if (*current == prev_char) {
            count++;
        } else {
            *dest_ptr++ = prev_char;
            char count_str[20];
            sprintf(count_str, "%d", count);
            for (int i = 0; count_str[i] != '\0'; i++) {
                *dest_ptr++ = count_str[i];
            }
            prev_char = *current;
            count = 1;
        }
    }

    *dest_ptr++ = prev_char;
    char count_str[20];
    sprintf(count_str, "%d", count);
    for (int i = 0; count_str[i] != '\0'; i++) {
        *dest_ptr++ = count_str[i];
    }
    *dest_ptr = '\0';
}