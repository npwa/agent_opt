#include <string.h>

void compress_string(const char *src, char *dest)
{
    int idx = 0;       /* index into dest (next write position) */
    const char *p;     /* scans src forward */
    const char *run_start;  /* start of the current run we're examining in src */

    for (run_start = src; *run_start != '\0'; ) {
        p = run_start + 1;            /* first candidate that may differ from the character we are counting */
        while (*p != '\0' && *p == *run_start)
            ++p;                        /* advance past all identical characters */

        dest[idx++] = *run_start;      /* store the run's character          */
                                      /* store the run length in decimal */
        int count = (int)(p - run_start);
        char buf[16];
        sprintf(buf, "%d", count);
        for (const char *s = buf; *s != '\0'; ++s)
            dest[idx++] = *s;          /* copy each digit into dest       */

        run_start = p;                 /* advance to the next run start    */
    }

    dest[idx] = '\0';                  /* null-terminate                   */
}
