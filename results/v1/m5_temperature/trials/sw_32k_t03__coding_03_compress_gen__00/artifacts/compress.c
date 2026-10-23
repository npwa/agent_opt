void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *run_start = src;

    while (*run_start) {
        const char *run_end = run_start + 1;
        while (*run_end && *run_end == *run_start)
            run_end++;

        int count = (int)(run_end - run_start);
        *dest++ = *run_start;

        char buf[16];
        int len = 0;
        int tmp = count;
        do {
            buf[len++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp);

        while (len--)
            *dest++ = buf[len];

        run_start = run_end;
    }

    *dest = '\0';
}
