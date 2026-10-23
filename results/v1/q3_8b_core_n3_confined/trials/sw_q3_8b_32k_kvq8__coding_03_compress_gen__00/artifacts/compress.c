#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *p = src;
    char current_char = *p;
    int count = 1;
    char *q = dest;

    while (*++p) {
        if (*p == current_char) {
            count++;
        } else {
            *q++ = current_char;
            int len = snprintf(q, 10, "%d", count);
            q += len;
            current_char = *p;
            count = 1;
        }
    }

    *q++ = current_char;
    int len = snprintf(q, 10, "%d", count);
    q += len;
    *q = '\0';
}
