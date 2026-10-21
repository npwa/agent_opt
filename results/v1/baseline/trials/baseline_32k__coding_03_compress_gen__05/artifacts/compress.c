#include <string.h>

void compress_string(const char *src, char *dest) {
    const char *ptr = src;
    int count = 0;
    char prev = '\0';
    int first = 1;

    while (*ptr != '\0') {
        if (first || *ptr == prev) {
            if (first) {
                prev = *ptr;
                count = 1;
                first = 0;
            } else {
                count++;
            }
        } else {
            *dest++ = prev;
            dest += sprintf(dest, "%d", count);
            prev = *ptr;
            count = 1;
        }
        ptr++;
    }

    if (!first) {
        *dest++ = prev;
        dest += sprintf(dest, "%d", count);
    }
    *dest = '\0';
}
