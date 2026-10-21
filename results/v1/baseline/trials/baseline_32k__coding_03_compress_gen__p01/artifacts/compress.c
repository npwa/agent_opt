#include <string.h>

void compress_string(const char *src, char *dest)
{
    int i = 0;
    while (src[i] != '\0') {
        char current = src[i];
        int count = 0;
        while (src[i] == current && src[i] != '\0') {
            count++;
            i++;
        }
        *dest++ = current;
        dest += sprintf(dest, "%d", count);
    }
    *dest = '\0';
}
