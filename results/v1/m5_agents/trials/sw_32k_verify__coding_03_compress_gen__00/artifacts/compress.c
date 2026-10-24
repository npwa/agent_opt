#include <string.h>

void compress_string(const char *src, char *dest)
{
    const char *p = src;

    while (*p != '\0') {
        char c = *p;
        int count = 0;

        /* Count consecutive occurrences of c */
        while (*p == c) {
            count++;
            p++;
        }

        /* Write the character */
        *dest++ = c;

        /* Write the count as a string (handles counts >= 1) */
        char buf[16];
        int i = 0;
        do {
            buf[i++] = (count % 10) + '0';
            count /= 10;
        } while (count > 0);
        buf[i] = '\0';

        /* Write digits in reverse order */
        for (int j = i - 1; j >= 0; j--) {
            *dest++ = buf[j];
        }
    }

    *dest = '\0';
}
