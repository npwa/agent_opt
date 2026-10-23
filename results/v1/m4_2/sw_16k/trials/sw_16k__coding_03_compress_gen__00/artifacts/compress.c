#include <string.h>

void compress_string(const char *src, char *dest) {
    int len = strlen(src);
    if (len == 0) {
        *dest = '\0';
        return;
    }

    const char *p = src;
    int count = 1;
    char *d = dest;

    while (*p) {
        if (*(p + 1) == *p) {
            count++;
            p++;
        } else {
            *d++ = *p;
            d += sprintf(d, "%d", count);
            count = 1;
            p++;
        }
    }
    *d = '\0';
}
