#include <string.h>

void compress_string(const char *src, char *dest) {
    unsigned int i = 0;
    while (src[i] != '\0') {
        char c = src[i];
        unsigned int count = 1;
        while (src[i + count] == c)
            count++;

        *dest++ = c;
        dest += sprintf(dest, "%u", count);

        i += count;
    }
    *dest = '\0';
}
