#include <stdlib.h>
#include <stdio.h>

void compress_string(const char *src, char *dest)
{
    if (src == NULL || dest == NULL)
        return;

    const char *ptr = src;
    int count = 0;
    char prev = '\0';

    while (*ptr != '\0') {
        if (*ptr == prev) {
            count++;
        } else {
            if (count > 0) {
                *dest++ = prev;
                dest += sprintf(dest, "%d", count);
            }
            prev = *ptr;
            count = 1;
        }
        ptr++;
    }

    if (count > 0) {
        *dest++ = prev;
        dest += sprintf(dest, "%d", count);
    }

    *dest = '\0';
}
