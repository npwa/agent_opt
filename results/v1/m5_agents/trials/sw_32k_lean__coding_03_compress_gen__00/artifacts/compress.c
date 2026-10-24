#include <stddef.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    const char *run_start = src;

    while (*run_start != '\0') {
        char current_char = *run_start;
        int count = 0;

        /* Count consecutive identical characters */
        while (*run_start == current_char) {
            count++;
            run_start++;
        }

        /* Write character, then its decimal count */
        *dest++ = current_char;

        if (count >= 10) {
            /* Multi-digit: write all digits manually */
            char buf[16];
            int pos = 0;
            int tmp = count;
            while (tmp > 0) {
                buf[pos++] = (tmp % 10) + '0';
                tmp /= 10;
            }
            /* Write digits in correct order */
            for (int i = pos - 1; i >= 0; --i)
                *dest++ = buf[i];
        } else {
            *dest++ = count + '0';
        }
    }

    *dest = '\0';
}
