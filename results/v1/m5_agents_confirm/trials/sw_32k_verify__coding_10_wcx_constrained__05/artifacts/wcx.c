/*
 * wcx: count lines, words, bytes, and the longest line.
 *
 * Output:  <lines> <words> <bytes> <maxlen>\n
 *
 * lines      = number of '\n' in input (last line without trailing '\n'
 *              does *not* increment lines per spec)
 * words      = maximal runs of non-whitespace bytes;
 *              whitespace = space \t \n \v \f \r (exactly six bytes)
 * bytes      = total bytes read from stdin
 * maxlen     = longest line in bytes, not counting its terminating '\n';
 *              a final line without trailing '\n' counts as a line
 */

#include <stdio.h>

static inline int is_ws(int c)
{
    return (c == ' ' || c == '\t'  || c == '\n' ||
            c == '\v' || c == '\f'  || c == '\r') ? 1 : 0;
}

int main(void)
{
    int ch;
    long lines   = 0;
    long words   = 0;
    long nbytes  = 0;
    long maxlen  = 0;
    int  in_word = 0;     /* are we currently inside a word? */
    long linelen = 0;     /* bytes of the current line so far (not counting \n) */

    while ((ch = getchar()) != EOF) {
        nbytes++;

        /* ---- lines ---- */
        if (ch == '\n')
            lines++;

        /* ---- words ---- */
        if (is_ws(ch)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }

        /* ---- maxlen ---- */
        linelen++;
        if (ch == '\n') {
            if (linelen > maxlen)
                maxlen = linelen;
            linelen = 0;
        }
    }

    /* If the file ends without a trailing newline, the last line is still
     * counted for maxlen. */
    if (linelen > maxlen)
        maxlen = linelen;

    printf("%ld %ld %ld %ld\n", lines, words, nbytes, maxlen);
    return 0;
}
