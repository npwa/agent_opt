#include <stddef.h>
#include <stdio.h>

void compress_string(const char *src, char *dest)
{
    size_t i, count;
    char prev;

    if (!src || !dest)
        return;

    if (*src == '\0') {
        *dest = '\0';
        return;
    }

    prev = *src++;
    count = 1;

    while (*src != '\0') {
        if (*src == prev) {
            count++;
        } else {
            *dest++ = prev;
            dest += sprintf(dest, "%zu", count);
            prev = *src;
            count = 1;
        }
        src++;
    }

    *dest++ = prev;
    dest += sprintf(dest, "%zu", count);
    *dest = '\0';
}
