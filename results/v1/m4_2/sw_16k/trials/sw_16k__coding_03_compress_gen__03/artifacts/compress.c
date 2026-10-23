void compress_string(const char *src, char *dest) {
    const char *p = src;
    char *d = dest;

    while (*p) {
        char c = *p;
        int count = 0;

        while (*p == c) {
            count++;
            p++;
        }

        *d++ = c;
        d += sprintf(d, "%d", count);
    }

    *d = '\0';
}
