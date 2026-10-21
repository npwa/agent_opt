#include <stdio.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *p = src;
    int count = 0;
    char prev = '\0';

    while (*p) {
        if (count == 0) {
            prev = *p;
            count = 1;
        } else if (*p == prev) {
            count++;
        } else {
            *dest++ = prev;
            dest += sprintf(dest, "%d", count);
            prev = *p;
            count = 1;
        }
        p++;
    }

    if (count > 0) {
        *dest++ = prev;
        dest += sprintf(dest, "%d", count);
    }

    *dest = '\0';
}
