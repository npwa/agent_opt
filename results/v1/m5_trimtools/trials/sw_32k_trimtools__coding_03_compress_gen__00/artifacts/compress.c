#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (src == NULL || dest == NULL) return;

    const char *run_start = src;
    while (*run_start != '\0') {
        const char *run_end = run_start;
        while (*run_end == *run_start && *run_end != '\0') {
            run_end++;
        }
        int count = (int)(run_end - run_start);

        *dest++ = *run_start;
        if (count > 9) {
            char buf[16];
            int pos = 0;
            while (count > 0) {
                buf[pos++] = (char)('0' + (count % 10));
                count /= 10;
            }
            while (pos > 0) {
                *dest++ = buf[--pos];
            }
        } else if (count == 1) {
            *dest++ = '1';
        } else {
            *dest++ = (char)('0' + count);
        }

        run_start = run_end;
    }

    *dest = '\0';
}
