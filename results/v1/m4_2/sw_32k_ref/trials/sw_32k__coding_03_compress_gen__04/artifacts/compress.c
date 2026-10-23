#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *p = src;
    char *d = dest;
    int count;
    char current;

    while (*p) {
        current = *p;
        count = 0;

        while (*p == current) {
            count++;
            p++;
        }

        *d++ = current;
        d += sprintf(d, "%d", count);
    }

    *d = '\0';
}
