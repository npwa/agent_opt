#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(void) {
    char buf[4096];
    unsigned long long lines = 0, words = 0, bytes = 0, maxlen = 0;
    int in_word = 0;
    while (fgets(buf, sizeof buf, stdin)) {
        size_t n = strlen(buf);
        bytes += n;
        size_t len = n;
        if (n && buf[n - 1] == '\n') { lines++; len--; }
        if (len > maxlen) maxlen = len;
        for (size_t i = 0; i < n; i++) {
            if (isspace((unsigned char)buf[i])) in_word = 0;
            else if (!in_word) { in_word = 1; words++; }
        }
    }
    printf("%llu %llu %llu %llu\n", lines, words, bytes, maxlen);
    return 0;
}
