#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    int i = 0;
    int j = 0;
    char current_char = src[0];
    int count = 1;

    while (src[i] != '\0') {
        if (src[i] == current_char) {
            count++;
        } else {
            dest[j++] = current_char;
            snprintf(dest + j, 10, "%d", count);
            j += strlen(dest + j);
            current_char = src[i];
            count = 1;
        }
        i++;
    }

    dest[j++] = current_char;
    snprintf(dest + j, 10, "%d", count);
    dest[j] = '\0';
}