#include <stdio.h>
void compress_string(const char *src, char *dest) {
    while (*src) {
        char c = *src;
        int n = 0;
        while (*src == c) { n++; src++; }
        dest += sprintf(dest, "%c%d", c, n);
    }
    *dest = '\0';
}
