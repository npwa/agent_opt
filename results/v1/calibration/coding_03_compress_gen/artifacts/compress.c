#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest)
{
    int len = strlen(src);
    if (len == 0) {
        *dest = '\0';
        return;
    }

    int i = 0;
    while (i < len) {
        char ch = src[i];
        int count = 1;
        while (i + count < len && src[i + count] == ch) {
            count++;
        }

        *dest++ = ch;
        dest += sprintf(dest, "%d", count);
        i += count;
    }

    *dest = '\0';
}