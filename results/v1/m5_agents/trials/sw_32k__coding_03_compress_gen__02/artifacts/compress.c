#include <stdio.h>
#include <string.h>

void compress_string(const char *src, char *dest)
{
    const char *p = src;
    char *d = dest;

    while (*p) {
        char c = *p;
        int count = 0;
        while (*p == c) {
            count++;
            p++;
        }
        *d++ = c;
        d += sprintf(d, "%d", count);
    }
    *d = '\0';
}

int main(void)
{
    const char *tests[] = {
        "aabcccccaaa",
        "abc",
        "aaa",
        "a",
        "aabbcc",
        "",
        NULL
    };

    for (int i = 0; tests[i] != NULL; i++) {
        char buf[256];
        compress_string(tests[i], buf);
        printf("\"%s\" -> \"%s\"\n", tests[i], buf);
    }
    return 0;
}
