#include <stdio.h>

void compress_string(const char *src, char *dest)
{
    size_t i = 0;

    while (src[i]) {
        char c = src[i];
        int count = 0;

        while (src[i] == c) {
            count++;
            i++;
        }

        *dest++ = c;
        dest += sprintf(dest, "%d", count);
    }

    *dest = '\0';
}
