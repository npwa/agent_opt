#include <stdio.h>

/* Return 1 if byte b is one of the six whitespace bytes, 0 otherwise. */
static int ws(unsigned char b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\v' ||
           b == '\f'   || b == '\r';
}

int main(void) {
    size_t lines = 0;      /* count of \n bytes */
    size_t words = 0;      /* maximal runs of non-whitespace bytes */
    size_t bytes = 0;      /* total bytes read */
    size_t maxlen = 0;     /* length of longest line (not counting its \n) */

    size_t cur_line = 0;   /* current line's running length */
    int in_word = 0;       /* inside a word? (1=yes, 0=no) */

    {
        int c;
        while ((c = fgetc(stdin)) != EOF) {
            unsigned char b = (unsigned char)c;

            if (b == '\n') {
                lines++;
                bytes++;
                cur_line = 0;
                in_word = 0;
            } else {
                bytes++;
                cur_line++;
                if (cur_line > maxlen)
                    maxlen = cur_line;
                /* Word counting: "enter a word" model. */
                if (!ws(b)) {           /* current byte is non-whitespace */
                    if (!in_word)       /* we just left whitespace or started input */
                        words++;      /* start of a new run */
                    in_word = 1;
                } else {
                    in_word    = 0;   /* hit a whitespace byte (space/tab/\v/\f/\r) */
                }
            }
        }

        /* The last line: if its bytes are non-zero, it's a partial line
           without a trailing \n. We already counted its maxlen when we
           scanned forward; no extra lines count for this (lines = # of \n). */
    }

    printf("%zu %zu %zu %zu\n", lines, words, bytes, maxlen);
    return 0;
}
