void compress_string(const char *src, char *dest) {
    const char *p = src;

    while (*p) {
        char ch = *p;
        int count = 0;

        while (*p == ch) {
            count++;
            p++;
        }

        *dest++ = ch;

        if (count >= 100) {
            *dest++ = (char)('0' + count / 100);
            count %= 100;
        }
        if (count >= 10) {
            *dest++ = (char)('0' + count / 10);
            count %= 10;
        }
        *dest++ = (char)('0' + count);
    }

    *dest = '\0';
}
