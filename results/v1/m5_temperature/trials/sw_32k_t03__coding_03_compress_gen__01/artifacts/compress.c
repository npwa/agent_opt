#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *run_start = src;
    char prev = 0;
    int count = 0;
    int idx = 0;

    while (*run_start) {
        if (*run_start == prev) {
            count++;
        } else {
            if (prev) {
                dest[idx++] = prev;
                char buf[16];
                int len = 0;
                int tmp = count;
                while (tmp) {
                    buf[len++] = '0' + (tmp % 10);
                    tmp /= 10;
                }
                while (len > 0) {
                    dest[idx++] = buf[--len];
                }
            }
            prev = *run_start;
            count = 1;
        }
        run_start++;
    }

    if (prev) {
        dest[idx++] = prev;
        char buf[16];
        int len = 0;
        int tmp = count;
        while (tmp) {
            buf[len++] = '0' + (tmp % 10);
            tmp /= 10;
        }
        while (len > 0) {
            dest[idx++] = buf[--len];
        }
    }

    dest[idx] = '\0';
}
