#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

void compress_string(const char *src, char *dest) {
    if (src == NULL || dest == NULL) return;

    int len = strlen(src);
    if (len == 0) {
        dest[0] = '\0';
        return;
    }

    int j = 0;
    int i = 0;
    
    while (i < len) {
        char current_char = src[i];
        int count = 1;

        while (i + 1 < len && src[i + 1] == current_char) {
            count++;
            i++;
        }

        dest[j++] = current_char;

        if (count >= 100) {
            dest[j++] = (char)('0' + count / 100 % 10);
            dest[j++] = (char)('0' + count / 10 % 10);
            dest[j++] = (char)('0' + count % 10);
        } else if (count >= 10) {
            dest[j++] = (char)('0' + count / 10);
            dest[j++] = (char)('0' + count % 10);
        } else {
            dest[j++] = (char)('0' + count);
        }

        i++;
    }

    dest[j] = '\0';
}
