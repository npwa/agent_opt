#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *p = src;
    char *d = dest;

    while (*p) {
        char c = *p;
        int count = 0;
        while (*p == c) {
            count++;
            p++;
        }
        *d++ = c;
        int digits[10];
        int n = 0;
        if (count == 0) {
            digits[n++] = 0;
        } else {
            while (count > 0) {
                digits[n++] = count % 10;
                count /= 10;
            }
        }
        while (n > 0) {
            *d++ = digits[--n] + '0';
        }
    }
    *d = '\0';
}
