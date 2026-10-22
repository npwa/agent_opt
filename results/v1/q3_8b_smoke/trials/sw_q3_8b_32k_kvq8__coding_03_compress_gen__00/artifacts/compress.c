#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (src == NULL || dest == NULL) {
        return;
    }
    if (*src == '\0') {
        *dest = '\0';
        return;
    }
    char current_char = src[0];
    int count = 1;
    char *dest_ptr = dest;
    for (int i = 1; src[i] != '\0'; i++) {
        if (src[i] == current_char) {
            count++;
        } else {
            *dest_ptr++ = current_char;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", count);
            int len = strlen(count_str);
            for (int j = 0; j < len; j++) {
                *dest_ptr++ = count_str[j];
            }
            current_char = src[i];
            count = 1;
        }
    }
    // Append the last character and count
    *dest_ptr++ = current_char;
    char count_str[10];
    snprintf(count_str, sizeof(count_str), "%d", count);
    int len = strlen(count_str);
    for (int j = 0; j < len; j++) {
        *dest_ptr++ = count_str[j];
    }
    *dest_ptr = '\0';
}
