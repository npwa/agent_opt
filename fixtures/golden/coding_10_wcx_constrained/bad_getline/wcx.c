#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(void) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    unsigned long long lines = 0, words = 0, bytes = 0, maxlen = 0;
    while ((n = getline(&line, &cap, stdin)) != -1) {
        bytes += (unsigned long long)n;
        int len = (int)n;
        if (len > 0 && line[len - 1] == '\n') { lines++; len--; }
        if ((unsigned long long)len > maxlen) maxlen = (unsigned long long)len;
        int in_word = 0;
        for (ssize_t i = 0; i < n; i++) {
            if (isspace((unsigned char)line[i])) in_word = 0;
            else if (!in_word) { in_word = 1; words++; }
        }
    }
    free(line);
    printf("%llu %llu %llu %llu\n", lines, words, bytes, maxlen);
    return 0;
}
