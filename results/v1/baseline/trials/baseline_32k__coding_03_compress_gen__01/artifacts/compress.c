void compress_string(const char *src, char *dest)
{
    if (!src) {
        *dest = '\0';
        return;
    }

    while (*src) {
        char prev = *src;
        int count = 0;
        while (*src && *src == prev) {
            count++;
            src++;
        }
        *dest++ = prev;
        dest += sprintf(dest, "%d", count);
    }
    *dest = '\0';
}
