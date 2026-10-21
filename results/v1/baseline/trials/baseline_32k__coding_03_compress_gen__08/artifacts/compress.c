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
        if (count >= 10) {
            // Two digits: tens and units
            *dest++ = (char)(count / 10 + '0');
        }
        *dest++ = (char)((count % 10) + '0');
    }
    *dest = '\0';
}
