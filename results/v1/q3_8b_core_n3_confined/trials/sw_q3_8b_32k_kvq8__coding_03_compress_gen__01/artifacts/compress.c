#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;
    const char *p = src;
    char current_char = *p;
    int count = 1;
    char *q = dest;

    p++;
    while (*p) {
        if (*p == current_char) {
            count++;
        } else {
            *q++ = current_char;
            sprintf(q, "%d", count);
            q += strlen(q);
            current_char = *p;
            count = 1;
        }
        p++;
    }

    // Handle the last character
    *q++ = current_char;
    sprintf(q, "%d", count);
    q += strlen(q);
    *q = '\0';
}
