#include <string.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *p = src;
    char *d = dest;

    while (*p) {
        char ch = *p;
        int count = 0;

        while (*p == ch) {
            count++;
            p++;
        }

        *d++ = ch;
        int tens = count / 10;
        int ones = count % 10;

        if (tens > 0) {
            *d++ = tens + '0';
        }
        *d++ = ones + '0';
    }

    *d = '\0';
}
