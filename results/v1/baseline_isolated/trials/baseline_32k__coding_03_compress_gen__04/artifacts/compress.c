#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest) {
    if (!src || !dest)
        return;

    const char *curr = src;
    int len = (int)strlen(src);
    int j = 0;

    while (curr - src < len) {
        char ch = *curr;
        int count = 0;

        while (*curr == ch && curr - src < len) {
            count++;
            curr++;
        }

        dest[j++] = ch;

        if (count >= 10) {
            int digits[6];
            int dlen = 0;
            int temp = count;
            while (temp > 0) {
                digits[dlen++] = temp % 10;
                temp /= 10;
                if (dlen >= 6) break;
            }
            for (int i = dlen - 1; i >= 0; i--) {
                dest[j++] = digits[i] + '0';
            }
        } else {
            dest[j++] = count + '0';
        }
    }

    dest[j] = '\0';
}

int main(void) {
    char buf[256];

    compress_string("aabcccccaaa", buf);
    printf("%s\n", buf);

    compress_string("abc", buf);
    printf("%s\n", buf);

    compress_string("hello world", buf);
    printf("%s\n", buf);

    return 0;
}
