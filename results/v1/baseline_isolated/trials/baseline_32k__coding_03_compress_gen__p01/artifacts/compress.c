#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

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
        d += sprintf(d, "%d", count);
    }
    *d = '\0';
}
