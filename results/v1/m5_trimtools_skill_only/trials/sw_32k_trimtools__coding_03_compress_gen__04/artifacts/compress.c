#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src) return;

    while (*src) {
        char ch = *src;
        int count = 0;

        while (*src == ch) {
            count++;
            src++;
        }

        *dest++ = ch;

        if (count > 9) {
            sprintf(dest, "%d", count);
            dest += strlen(dest);
        } else {
            *dest++ = (char)('0' + count);
        }
    }

    *dest = '\0';
}
