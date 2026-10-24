void compress_string(const char *src, char *dest) {
    while (*src) {
        char c = *src;
        int count = 0;
        while (*src == c) {
            count++;
            src++;
        }
        *dest++ = c;
        if (count >= 100) {
            *dest++ = (count / 100) + '0';
            count %= 100;
        }
        if (count >= 10) {
            *dest++ = (count / 10) + '0';
            count %= 10;
        }
        *dest++ = count + '0';
    }
    *dest = '\0';
}
