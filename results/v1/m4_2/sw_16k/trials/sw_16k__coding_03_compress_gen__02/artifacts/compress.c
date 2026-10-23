void compress_string(const char *src, char *dest) {
    while (*src) {
        char ch = *src;
        int count = 0;
        while (*src == ch) {
            count++;
            src++;
        }
        *dest++ = ch;
        if (count >= 10) {
            char tmp[16];
            int i = 0;
            while (count > 0) {
                tmp[i++] = '0' + (count % 10);
                count /= 10;
            }
            while (i--) *dest++ = tmp[i];
        } else {
            *dest++ = '0' + count;
        }
    }
    *dest = '\0';
}
