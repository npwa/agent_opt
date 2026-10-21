#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    while (*src) {
        char current = *src;
        int count = 0;

        while (*src == current) {
            count++;
            src++;
        }

        *dest++ = current;
        dest += snprintf(dest, 16, "%d", count);
    }

    *dest = '\0';
}
