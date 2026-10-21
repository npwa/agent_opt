#include <string.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest) {
        *dest = '\0';
        return;
    }

    const char *run_start = src;
    char *d = dest;

    while (*run_start) {
        char ch = *run_start;
        int count = 0;

        while (*run_start == ch) {
            count++;
            run_start++;
        }

        char num_buf[12];
        int n = snprintf(num_buf, sizeof(num_buf), "%d", count);
        memcpy(d, ch, 1);
        d++;
        memcpy(d, num_buf, n);
        d += n;
    }

    *d = '\0';
}
