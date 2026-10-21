void compress_string(const char *src, char *dest) {
    const char *p = src;
    while (*p) {
        char cur = *p;
        int count = 0;
        while (*p == cur) {
            count++;
            p++;
        }
        *dest++ = cur;
        char buf[16];
        int n = 0;
        do {
            buf[n++] = (count % 10) + '0';
            count /= 10;
        } while (count > 0);
        while (--n >= 0)
            *dest++ = buf[n];
    }
    *dest = '\0';
}
