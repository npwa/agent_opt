#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *ptr = src;
    int len = (int)strlen(src);

    if (len == 0) {
        dest[0] = '\0';
        return;
    }

    char *dst = dest;
    char prev = *ptr;
    int count = 1;

    ptr++;
    while (ptr < src + len) {
        if (*ptr == prev) {
            count++;
        } else {
            *dst++ = prev;
            dst += sprintf(dst, "%d", count);
            prev = *ptr;
            count = 1;
        }
        ptr++;
    }

    *dst++ = prev;
    dst += sprintf(dst, "%d", count);
    *dst = '\0';
}
