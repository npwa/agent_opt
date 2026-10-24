#include <stddef.h>

void compress_string(const char *src, char *dest)
{
    if (src == NULL || dest == NULL)
        return;

    const char *run = src;

    while (*run != '\0') {
        char ch = *run;
        int count = 0;

        /* Count consecutive occurrences of ch */
        while (*run == ch && *run != '\0') {
            count++;
            run++;
        }

        /* Write the character */
        *dest++ = ch;

        /* Convert count to decimal digits — write most-significant first */
        char buf[16];
        int pos = 0;
        int tmp = count;
        do {
            buf[pos++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);

        for (int i = pos - 1; i >= 0; i--)
            *dest++ = buf[i];
    }

    *dest = '\0';
}
