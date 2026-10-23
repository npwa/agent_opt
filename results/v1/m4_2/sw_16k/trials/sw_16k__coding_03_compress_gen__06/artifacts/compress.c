#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *run = src;
    char prev = '\0';
    int count = 0;

    while (*src) {
        if (*src == prev) {
            count++;
        } else {
            if (prev != '\0') {
                *dest++ = prev;
                // Convert count to ASCII digits
                char buf[16];
                int len = 0;
                int tmp = count;
                while (tmp > 0) {
                    buf[len++] = (tmp % 10) + '0';
                    tmp /= 10;
                }
                for (int i = len - 1; i >= 0; i--) {
                    *dest++ = buf[i];
                }
            }
            prev = *src;
            count = 1;
        }
        src++;
    }

    // Emit the last character group
    if (prev != '\0') {
        *dest++ = prev;
        char buf[16];
        int len = 0;
        int tmp = count;
        while (tmp > 0) {
            buf[len++] = (tmp % 10) + '0';
            tmp /= 10;
        }
        for (int i = len - 1; i >= 0; i--) {
            *dest++ = buf[i];
        }
    }

    *dest = '\0';
}
