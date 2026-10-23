void compress_string(const char *src, char *dest)
{
    const char *p = src;
    while (*p) {
        char ch = *p;
        int count = 0;
        while (*p == ch) {
            ++count;
            ++p;
        }
        *dest++ = ch;
        /* write count as decimal digits */
        int tmp[12];
        int n = 0;
        do {
            tmp[n++] = (char)((count % 10) + '0');
            count /= 10;
        } while (count > 0);
        while (--n >= 0) {
            *dest++ = tmp[n];
        }
    }
    *dest = '\0';
}
