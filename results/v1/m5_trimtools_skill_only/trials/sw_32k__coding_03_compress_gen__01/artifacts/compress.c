#include <string.h>

void compress_string(const char *src, char *dest)
{
    if (!src || !dest)
        return;

    const char *read = src;
    char *write = dest;

    while (*read) {
        char current = *read;
        int count = 0;

        /* count consecutive identical characters */
        while (*read == current) {
            count++;
            read++;
        }

        /* write character as-is (it's guaranteed alphabetical, single byte) */
        *write++ = current;

        /* write count in decimal */
        if (count >= 10) {
            char buf[8];
            int pos = 0;
            int temp = count;

            while (temp > 0) {
                buf[pos++] = '0' + (temp % 10);
                temp /= 10;
            }
            /* write in reverse */
            while (pos > 0) {
                *write++ = buf[--pos];
            }
        } else {
            *write++ = '0' + count;
        }
    }

    *write = '\0';
}
