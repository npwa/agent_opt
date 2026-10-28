#include <string.h>
#include <stdio.h>

void compress_string(const char *src, char *dest)
{
    const char *p = src;

    while (*p) {
        char c = *p;
        int count = 0;

        while (*p == c && *p != '\0') {
            count++;
            p++;
        }

        dest += sprintf(dest, "%c%d", c, count);
    }

    *dest = '\0';
}
