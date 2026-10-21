#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest) return;

    const char *start = src;
    int count = 1;

    while (*src) {
        if (*(src + 1) != *src) {
            /* write the character */
            *dest++ = *src;
            /* write decimal count */
            char buf[12];
            int len = 0;
            do {
                buf[len++] = (count % 10) + '0';
                count /= 10;
            } while (count > 0);
            while (--len >= 0) *dest++ = buf[len];
        }
        src++;
    }

    *dest = '\0';
}
