#include <stdio.h>
#include <string.h>
#include <assert.h>

void compress_string(const char *src, char *dest)
{
    size_t i = 0;

    while (src[i]) {
        char c = src[i];
        int count = 0;

        while (src[i] == c) {
            count++;
            i++;
        }

        *dest++ = c;
        dest += sprintf(dest, "%d", count);
    }

    *dest = '\0';
}

int main() {
    char buf[128];
    
    compress_string("aabcccccaaa", buf);
    printf("'%s' (expected 'a2b1c5a3')\n", buf);
    assert(strcmp(buf, "a2b1c5a3") == 0);

    compress_string("abc", buf);
    printf("'%s' (expected 'a1b1c1')\n", buf);
    assert(strcmp(buf, "a1b1c1") == 0);

    compress_string("aaa", buf);
    printf("'%s' (expected 'a3')\n", buf);
    assert(strcmp(buf, "a3") == 0);

    compress_string("", buf);
    printf("'%s' (expected '')\n", buf);
    assert(strlen(buf) == 0);

    printf("All tests passed!\n");
    return 0;
}
