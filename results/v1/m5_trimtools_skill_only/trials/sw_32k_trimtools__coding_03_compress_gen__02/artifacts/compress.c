#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest) return;

    int src_len = strlen(src);
    if (src_len == 0) {
        dest[0] = '\0';
        return;
    }

    int dest_idx = 0;
    int i = 0;

    while (i < src_len) {
        char current_char = src[i];
        int count = 1;

        while (i + count < src_len && src[i + count] == current_char) {
            count++;
        }

        dest[dest_idx++] = current_char;

        if (count > 1) {
            char count_str[12];
            int num_digits = 0;
            int temp_count = count;
            while (temp_count > 0) {
                count_str[num_digits++] = (temp_count % 10) + '0';
                temp_count /= 10;
            }

            for (int d = num_digits - 1; d >= 0; d--) {
                dest[dest_idx++] = count_str[d];
            }
        }

        i += count;
    }

    dest[dest_idx] = '\0';
}
