#include <string.h>
#include <stdio.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *prev = src;
    int count = 1;

    while (*src) {
        if (*src == *prev) {
            /* continue counting */
        } else {
            *dest++ = *prev;
            dest += sprintf(dest, "%d", count);
            prev = src;
            count = 1;
        }
        src++;
    }

    /* write the last run */
    *dest++ = *prev;
    dest += sprintf(dest, "%d", count);

    *dest = '\0';
}
