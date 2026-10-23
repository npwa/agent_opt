#include <stddef.h>

void compress_string(const char *src, char *dest) {
    const char *run_start = src;

    while (*run_start != '\0') {
        char run_char = *run_start;
        const char *run_end = run_start;

        while (*run_end == run_char) {
            ++run_end;
        }

        *dest++ = run_char;

        int count = (int)(run_end - run_start);
        if (count >= 10) {
            *dest++ = '0' + count / 10;
        }
        *dest++ = '0' + (count % 10);

        run_start = run_end;
    }

    *dest = '\0';
}
