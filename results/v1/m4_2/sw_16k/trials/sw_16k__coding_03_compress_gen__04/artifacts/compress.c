void compress_string(const char *src, char *dest) {
    const char *p = src;

    while (*p) {
        char c = *p;
        int count = 0;

        while (*p == c) {
            count++;
            p++;
        }

        *dest++ = c;
        *dest++ = '0' + count;
    }

    *dest = '\0';
}
