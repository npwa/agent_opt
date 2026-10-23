#include <string.h>

void compress_string(const char *src, char *dest) {
    if (src == NULL || dest == NULL)
        return;

    int i = 0;
    int j = 0;

    while (src[i]) {
        char current = src[i];
        int count = 0;

        while (src[i] == current) {
            count++;
            i++;
        }

        dest[j++] = current;
        if (count > 1) {
            char buf[24];
            int len = snprintf(buf, sizeof(buf), "%d", count);
            for (int k = 0; k < len; k++) {
                dest[j++] = buf[k];
            }
        }
    }

    dest[j] = '\0';
}
