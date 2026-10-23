#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *run_start = src;

    while (*run_start) {
        const char *run_end = run_start + 1;

        /* Skip consecutive identical characters */
        while (*run_end && *run_end == *run_start) {
            run_end++;
        }

        *dest++ = *run_start;
        int count = (int)(run_end - run_start);

        if (count >= 10) {
            *dest++ = (char)('0' + count / 10);
            *dest++ = (char)('0' + count % 10);
        } else {
            *dest++ = (char)('0' + count);
        }

        run_start = run_end;
    }

    *dest = '\0';
}
