#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    int len = strlen(src);
    if (len == 0) {
        dest[0] = '\0';
        return;
    }

    const char *p = src;
    while (*p) {
        char c = *p;
        int count = 0;
        while (*p == c) {
            count++;
            p++;
        }
        *dest++ = c;
        if (count >= 10) {
            *dest++ = '0' + (count / 10);
        }
        *dest++ = '0' + (count % 10);
    }
    *dest = '\0';
}
