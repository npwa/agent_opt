#include <stddef.h>
#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *src_copy = src;
    size_t count;

    while (*src_copy != '\0') {
        char c = *src_copy++;
        count = 1;
        while (*src_copy == c) {
            src_copy++;
            count++;
        }
        dest += sprintf(dest, "%c%zu", c, count);
    }
    *dest = '\0';
}
