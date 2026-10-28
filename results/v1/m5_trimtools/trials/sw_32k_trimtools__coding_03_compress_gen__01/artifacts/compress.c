void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *cur = src;
    while (*cur) {
        char ch = *cur;
        int count = 0;
        while (*cur == ch) {
            count++;
            cur++;
        }
        *dest++ = ch;
        if (count >= 10) {
            int digits[10];
            int n = 0;
            do {
                digits[n++] = (count % 10);
                count /= 10;
            } while (count > 0);
            while (--n >= 0)
                *dest++ = '0' + digits[n];
        } else {
            *dest++ = '0' + count;
        }
    }
    *dest = '\0';
}
