#include <stdio.h>

int main(void)
{
    int lines = 0, words = 0, bytes_count = 0;
    int maxlen = 0;
    int curline = 0;
    int in_word = 0; /* 1 while currunning inside a word run */
    int c;
    int cr_pending = 0; /* track \r that may or may not be followed by \n */
    long cr_lines = 0;  /* count of lone \r (not followed by \n) as "lines" */

    while ((c = getchar()) != EOF) {
        unsigned char uc = (unsigned char)c;

        /* --- whitespace check: space, \t, \n, \v, \f, \r --- */
        if (uc == ' ' || uc == '\t' || uc == '\n' ||
            uc == '\v' || uc == '\f') {
            /* end any current word without counting it as a final line break */
            curline++;
            if (curline > maxlen)
                maxlen = curline;
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else if (uc == '\r') {
            /* \r is whitespace for word counting, and a word-ending byte */
            /* but for maxlen it's an "ordinary" (non-terminating) byte.
               For lines it counts as line-termination only if not followed by \n
               (i.e., old Mac-style CR-only line endings).

               We must lookahead: peek at the next byte.
            */
            int next = getchar(); /* this is safe; just consuming one more char */
            if (next != EOF) {
                unsigned char unc = (unsigned char)next;
                if (unc == '\n') {
                    /* \r\n pair -> CRLF line ending.  Count as line-terminating for lines,
                       but curline already got one incr from the \r-block above which is wrong. */

                    /* Undo that first block: we don't want to count \r in curline or maxlen
                       when it's part of CRLF.  Actually we do for curline (maxlen counts \r),
                       but "lines" should be counted exactly as if the \n did it. */
                } else {
                    /* lone CR: behaves both as whitespace (ending word) and line terminator
                       (for old-Mac compatibility, though spec doesn't mention this explicitly).
                       However, re-reading spec again... the only thing mentioned about lines is
                       counting \n bytes.  So \r alone does NOT increment "lines". */

                    /* We already incremented curline in the first if-block above for \r because it's ws.
                       Actually no \r falls outside that first condition. It goes here. */
                }
            }

            /* Since the lookahead ate 'next', we need to process it now too,
               and handle both cases from inside this elif branch.           */
        } else {
            /* non-whitespace byte (NUL through 0xFF exclusive of ws set) */
            if (!in_word)
                in_word = 1;
            curline++;
        }

        bytes_count++;
    }

    /* Handle remaining state at EOF */
    printf("%d %d %d %d\n", lines, words, bytes_count, maxlen);
    return 0;
}
