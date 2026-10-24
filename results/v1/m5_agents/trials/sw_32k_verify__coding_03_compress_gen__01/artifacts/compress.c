#include <string.h>
#include <stdio.h>

/*
 * compress_string - run-length compress src into dest
 *
 * Replaces consecutive repeated characters with the character
 * followed by its run-count.  Example: "aabcccccaaa" -> "a2b1c5a3".
 *
 * Assumptions:
 *   - src contains only a-zA-Z and is NULL-terminated
 *   - dest points to memory large enough for the compressed output
 */
void compress_string(const char *src, char *dst) {
    if (src == NULL || dst == NULL)
        return;

    const char *p = src;

    while (*p != '\0') {
        unsigned int count = 1;
        char ch = *p;

        /* Count consecutive identical characters */
        while (p[1] != '\0' && p[1] == ch) {
            ++count;
            ++p;
        }

        /* Write the character and its run-count */
        *dst++ = ch;

        if (count < 10) {
            *dst++ = '0' + (char)count;
        } else {
            /* Build digits in a small buffer, then write them */
            unsigned int tmp = count;
            char buf[20];
            int len = 0;

            do {
                buf[len++] = (char)('0' + (int)(tmp % 10));
                tmp /= 10;
            } while (tmp > 0);

            /* Write digits in reverse order */
            for (int i = len - 1; i >= 0; --i)
                *dst++ = buf[i];
        }

        ++p;
    }

    *dst = '\0';
}
