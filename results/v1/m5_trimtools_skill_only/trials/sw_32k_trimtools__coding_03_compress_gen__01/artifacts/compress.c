void compress_string(const char *src, char *dest) {
    int i = 0;
    while (src[i] != '\0') {
        char ch = src[i];
        int count = 0;
        while (src[i] == ch) {
            count++;
            i++;
        }
        *dest++ = ch;
        char buf[12];
        int j = 0;
        do {
            buf[j++] = '0' + (count % 10);
            count /= 10;
        } while (count > 0);
        while (j > 0) {
            *dest++ = buf[--j];
        }
    }
    *dest = '\0';
}
