#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    while (*src) {
        char c = *src;
        int count = 0;

        while (*src == c) {
            count++;
            src++;
        }

        *dest++ = c;
        char buf[16];
        int blen = snprintf(buf, sizeof(buf), "%d", count);
        for (int i = 0; i < blen; i++)
            *dest++ = buf[i];
    }

    *dest = '\0';
}
