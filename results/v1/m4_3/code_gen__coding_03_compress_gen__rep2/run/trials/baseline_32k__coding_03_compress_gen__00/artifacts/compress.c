#include <string.h>
#include <stdio.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;
    
    int count = 0;
    char current = src[0];
    
    while (*src) {
        if (*src == current) {
            count++;
        } else {
            dest += sprintf(dest, "%c%d", current, count);
            current = *src;
            count = 1;
        }
        src++;
    }
    dest += sprintf(dest, "%c%d", current, count);
    *dest = '\0';
}
