#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    int i = 0;
    int j = 0;
    while (src[i]) {
        char c = src[i];
        int count = 0;
        while (src[i] == c) {
            count++;
            i++;
        }
        dest[j++] = c;
        if (count > 1) {
            char buf[32];
            int n = snprintf(buf, sizeof(buf), "%d", count);
            for (int k = 0; k < n; k++)
                dest[j++] = buf[k];
        }
    }
    dest[j] = '\0';
}
