#include <string.h>

void compress_string(const char *src, char *dest) {
    int i = 0;
    int len = strlen(src);

    while (i < len) {
        char current = src[i];
        int count = 0;

        while (i < len && src[i] == current) {
            count++;
            i++;
        }

        *dest++ = current;

        if (count > 1) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%d", count);
            strcat(dest, buf);
            dest += strlen(buf);
        }
    }

    *dest = '\0';
}
