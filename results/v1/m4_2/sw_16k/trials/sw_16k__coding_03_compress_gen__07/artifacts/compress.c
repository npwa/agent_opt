#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *ptr = src;
    char *d = dest;

    while (*ptr) {
        char ch = *ptr;
        int count = 0;

        while (*ptr == ch && *ptr) {
            count++;
            ptr++;
        }

        *d++ = ch;
        if (count >= 1000) {
            *d++ = '0' + count / 100;
            count %= 100;
        }
        if (count >= 10) {
            *d++ = '0' + count / 10;
            count %= 10;
        }
        *d++ = '0' + count;
    }

    *d = '\0';
}
