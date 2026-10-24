#include <stdio.h>

#define BUF 65536

int main(void){
    unsigned char buf[BUF];
    long lines = 0, words = 0, bytes = 0;
    size_t maxlen = 0, curlen = 0;
    int in_word = 0;

    while (1){
        int n = (int)fread(buf, 1, BUF, stdin);
        if (n == 0) break;
        bytes += n;

        for (int i = 0; i < n; i++){
            unsigned char c = buf[i];

            if (c == '\n'){
                lines++;
                if (curlen > maxlen) maxlen = curlen;
                curlen = 0;
                in_word = 0;
            } else {
                curlen++;
                int is_ws = (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
                if (!is_ws){
                    if (!in_word) words++;
                    in_word = 1;
                } else {
                    in_word = 0;
                }
            }
        }
    }

    if (curlen > maxlen) maxlen = curlen;

    printf("%ld %ld %ld %zu\n", lines, words, bytes, maxlen);
    return 0;
}
