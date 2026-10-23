#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *run_start = src;
    char *out = dest;

    while (*run_start) {
        const char *run_end = run_start;
        char ch = *run_start;

        while (*run_end == ch)
            run_end++;

        int count = (int)(run_end - run_start);

        *out++ = ch;

        if (count >= 100) {
            *out++ = (char)('0' + count / 100 % 10);
            *out++ = (char)('0' + count / 10 % 10);
            *out++ = (char)('0' + count % 10);
        } else if (count >= 10) {
            *out++ = (char)('0' + count / 10 % 10);
            *out++ = (char)('0' + count % 10);
        } else {
            *out++ = (char)('0' + count);
        }

        run_start = run_end;
    }

    *out = '\0';
}
