void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *run_start = src;
    const char *run_end = src;

    while (*run_start) {
        while (*run_end == *run_start)
            run_end++;

        int count = (int)(run_end - run_start);

        /* Write the character */
        *dest++ = *run_start;

        /* Convert count to decimal digits and write them */
        char buf[32];
        int idx = 0;
        do {
            buf[idx++] = (char)('0' + (count % 10));
            count /= 10;
        } while (count > 0);

        /* Digits are in reverse order, write them back */
        while (idx > 0)
            *dest++ = buf[--idx];

        run_start = run_end;
    }

    *dest = '\0';
}
