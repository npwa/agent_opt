#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *p = src;
    int count = 0;

    while (*p) {
        char current = *p;
        count = 1;
        while (p[1] == current) {
            count++;
            p++;
        }
        *dest++ = current;
        dest += sprintf(dest, "%d", count);
        p++;
    }
    *dest = '\0';
}
