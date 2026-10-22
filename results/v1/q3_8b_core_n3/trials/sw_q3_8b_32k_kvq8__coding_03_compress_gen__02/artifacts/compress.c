void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    if (*src == '\0') {
        *dest = '\0';
        return;
    }

    char prev_char = *src;
    int count = 1;
    char *dest_ptr = dest;

    const char *current = src + 1;

    while (*current != '\0') {
        if (*current == prev_char) {
            count++;
        } else {
            *dest_ptr++ = prev_char;
            *dest_ptr++ = '0' + count;
            prev_char = *current;
            count = 1;
        }
        current++;
    }

    // Handle the last character
    *dest_ptr++ = prev_char;
    *dest_ptr++ = '0' + count;
    *dest_ptr = '\0';
}
