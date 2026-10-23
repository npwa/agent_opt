#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *run_start = src;

    while (*run_start) {
        const char *run_end = run_start + 1;

        while (*run_end && *run_end == *run_start)
            run_end++;

        size_t count = (size_t)(run_end - run_start);

        *dest++ = *run_start;

        if (count >= 10) {
            char buf[16];
            int n = 0;
            while (count > 0) {
                buf[n++] = (char)((count % 10) + '0');
                count /= 10;
            }
            while (n-- > 0)
                *dest++ = buf[n];
        } else {
            *dest++ = (char)(count + '0');
        }

        run_start = run_end;
    }

    *dest = '\0';
}
