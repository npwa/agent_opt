#include <stdio.h>

int main(void)
{
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long max_len = 0;
    long cur_len = 0;
    int in_word = 0;
    int ch;

    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF)
            break;
        bytes++;

        if (ch == '\n') {
            lines++;
            if (cur_len > max_len)
                max_len = cur_len;
            cur_len = 0;
            in_word = 0;
        } else if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r') {
            /* \r is counted in maxlen but not as a line separator */
            if (ch != '\r') {
                cur_len++;
                if (in_word) {
                    words++;
                    in_word = 0;
                }
            }
        } else {
            cur_len++;
            if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }

    /* A final line without trailing newline still counts */
    if (bytes > 0 && bytes != lines) {
        /* already handled above or this branch */
    }
    if (in_word || cur_len > 0) {
        if (!bytes && !lines) {
            /* empty input edge: handle below */
        }
    }

    Actually, let me redo the logic more carefully.

Actually let me rewrite with cleaner logic:


int main(void)
{
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long max_len = 0;
    long cur_len = 0;
    int in_word = 0;
    int ch;

    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF)
            break;
        bytes++;
        cur_len++;

        if (ch == '\n') {
            lines++;
            if (cur_len - 1 > max_len)
                max_len = cur_len - 1;
            cur_len = 0;
            in_word = 0;
        } else if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') {
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            /* \r counts toward maxlen but not as whitespace for word boundaries
               per spec: \r is an ordinary byte for maxlen, 
               and it's one of the six whitespace bytes for word counting */
            if (ch == '\r') {
                /* do nothing special - stays in whatever word state */
                /* actually re-read spec: \r IS one of the 6 whitespace bytes */
                /* so words are delimited by \r too, but cur_len includes \r */
            } else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }
}


Let me just rewrite cleanly and correctly per this specification

#include <stdio.h>

static int is_wspace(int ch)
{
    /* Return true for: space, \t, \n, \v, \f, \r */
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\v' ||
           ch == '\f' || ch == '\r';
}

int main(void)
{
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long max_len = 0;
    long cur_len = 0;
    int in_word = 0;
    int ch;

    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF)
            break;
        bytes++;

        /* \n ends a line */
        if (ch == '\n') {
            lines++;
            if (cur_len > max_len)
                max_len = cur_len;
            cur_len = 0;
            in_word = 0;
            continue;
        }

        /* For maxlen: \r is an ordinary byte, so always increment */
        cur_len++;

        /* For word counting */
        if (is_wspace(ch) && ch != '\r') {
            /* whitespace delimiters (not \r): end current word */
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else if (ch == '\r') {
            /* \r is whitespace for words, ends current word */
            if (in_word) {
                words++;
                in_word = 0;
            }
            /* but cur_len already incremented above */
        }

        /* For maxlen: \r counts as ordinary byte (already handled above) */
    }

    /* Handle last line that may not end with \n */
    if (bytes > 0 || max_len > 0) {
        if (cur_len > max_len)
            max_len = cur_len;
    }

    printf("%ld %ld %ld %ld\n", lines, words, bytes, max_len);
    return 0;
}


/*
The spec says words are "maximal runs of bytes that are NOT whitespace" where
whitespace = {space, \t, \n, \v, \f, \r}.

So \r DOES end words. And maxlen counts \r as an ordinary byte.

Key insight for line: every \n terminates a line. A final line without \n
doesn't get its \n but still "counts" for maxlen purposes (it has maxlen).

Let me think about the logic again for a simpler implementation using getline-
like buffering but without malloc - we use an auto buffer that grows, or better
yet, read char by char.
*/

#include <stdio.h>

int main(void)
{
    long lines = 0;
    long words = 0;
    long bytes = 0;
    long max_len = 0;
    long cur_len = 0;
    int in_word = 0;
    int ch;

    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF)
            break;
        bytes++;

        if (ch == '\n') {
            /* End of line: cur_len is the length not counting \n */
            lines++;
            if (cur_len > max_len)
                max_len = cur_len;
            cur_len = 0;
            in_word = 0;
        } else {
            /* \r counts toward maxlen */
            cur_len++;

            /* Words: maximal runs of non-whitespace bytes */
            if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') {
                if (in_word) {
                    words++;
                    in_word = 0;
                }
            } else if (ch == '\r') {
                /* \r is whitespace for word detection */
                if (in_word) {
                    words++;
                    in_word = 0;
                }
            } else {
                /* Non-whitespace byte */
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }

    /* Handle last line without trailing \n — it still counts */
    if (bytes > 0 && cur_len > max_len)
        max_len = cur_len;

    printf("%ld %ld %ld %ld\n", lines, words, bytes, max_len);
    return 0;
}


/* Wait — there's an issue. For maxlen: "A last line without a trailing newline 
 * counts as a line for this value." This means cur_len should still track it.
 * If we hit EOF on a non-\n char, cur_len accumulates but we never check it
 * against max_len. The final check handles that.
 *
 * But what about empty input? bytes=0, lines=0, words=0, max_len=0. OK.
 * What about input with just "\n"? bytes=1, after loop: cur_len=0, in_word=0.
 *   -> but inside the loop when ch='\n': lines++, max_len=max(0,0)=0,
 *      cur_len=0. Then bytes>0 && cur_len(0)>max_len(0) is false. No update.
 *   max_len stays 0 for a line with just \n... wait no. A line "\n" has one line
 *   of content "". The empty string has length 0. But the spec says "the longest 
 *   line, not counting its terminating \n". So "" = 0 is correct? No!
 *   Wait — the input "\n" has ONE line which is the empty string before \n. Its
 *   length is 0. That seems wrong... actually let me re-read: "the length in bytes
 *   of the longest line, not counting its terminating \n". A line containing just
 *   nothing before \n would have length 0. But wait, cur_len after seeing '\n' 
 *   should be 0 only if we reset BEFORE incrementing for content. Let me re-check:
 *   
 *   For "\n": ch='\n', cur_len=0 (never incremented since it's the first char).
 *   lines++, max_len=max(0,0)=0, cur_len=0. This seems OFF - the line length 
 *   before \n is 0 chars. Actually that IS correct for an empty line. A line 
 *   with content "abc\n" would have maxlen=3. A bare "\n" gives first (only)
 *   line length = 0. That's correct.
 *   
 *   But wait — what about "a\n"? ch='a': bytes=1, cur_len=1. Then '\n': 
 *   lines++, max_len=max(0,1)=1, cur_len=0. Correct, single char line has len 1. OK.
 */
