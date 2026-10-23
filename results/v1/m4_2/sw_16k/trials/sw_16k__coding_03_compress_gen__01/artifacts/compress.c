void compress_string(const char *src, char *dest)
{
    const char *run_start = src;
    int count;

    while (*src) {
        count = 0;
        while (*src == *run_start) {
            count++;
            src++;
        }
        *dest++ = *run_start;
        dest += sprintf(dest, "%d", count);
        run_start = src;
    }
    *dest = '\0';
}
