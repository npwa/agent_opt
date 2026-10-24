/* wcx: count lines, words, bytes and the longest line.
 * Reads stdin to EOF and prints  <lines> <words> <bytes> <maxlen>  followed by newline.
 * No dynamic memory — processes byte-by-byte so a 64 MiB line never overflows stack/heap.
 */

#include <stdio.h>

static int is_whitespace(int c)
{
    return (c == ' ') || (c == '\t') ||
           (c == '\v') || (c == '\f') ||
           (c == '\r');
}

int main(void)
{
    unsigned long lines = 0;
    unsigned long words = 0;
    unsigned long bytes = 0;
    unsigned long maxlen = 0;
    unsigned long current_len = 0;
    int in_word = 0;
    int c;

    while ((c = getchar()) != EOF) {
        bytes++;

        if (c == '\n') {
            /* Finalize the line we just read. */
            lines++;
            if (current_len > maxlen)
                maxlen = current_len;
            current_len = 0;
            in_word   = 0;
        } else if (is_whitespace(c)) {
            /* Space / tab / vertical-tab / form-feed / carriage-return. */
            if (in_word) {
                words++;
                in_word = 0;
            }
            current_len++;
        } else {
            /* Any byte except newline, space, \t, \v, \f, \r
               (includes NUL and bytes > 127). */
            if (!in_word) {
                words++;
                in_word = 1;
            }
            current_len++;
        }
    }

    /* A trailing line without a terminating newline still contributes to maxlen. */
    if (current_len > maxlen)
        maxlen = current_len;

    printf("%lu %lu %lu %lu\n", lines, words, bytes, maxlen);
    return 0;
}
