#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *run_start = src;

    while (*run_start) {
        char current_char = *run_start;
        const char *run_end = run_start + 1;

        /* Find the end of the current run */
        while (*run_end && *run_end == current_char) {
            run_end++;
        }

        /* Write the character */
        *dest++ = current_char;

        /* Calculate count and write digits */
        int count = (int)(run_end - run_start);
        int div = 1;
        while (count / (div * 10)) {
            div *= 10;
        }
        while (div) {
            *dest++ = '0' + count / div;
            div /= 10;
        }

        /* Move to the next run */
        run_start = run_end;
    }

    *dest = '\0';
}
