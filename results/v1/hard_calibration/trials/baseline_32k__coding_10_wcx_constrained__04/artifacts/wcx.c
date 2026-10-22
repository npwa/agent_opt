#include <stdio.h>
#include <stddef.h>

static void build_map(unsigned char *m)
{
    static const unsigned char ws[] = { ' ', '\t', '\n', '\v', '\f', '\r' };
    for (int i = 0; i < 256; i++)
        m[i] = 0;
    for (size_t i = 0; i < sizeof(ws); i++)
        m[ws[i]] = 1;
}

int main(void)
{
    unsigned char map[256];
    build_map(map);

    long nl = 0, wrds = 0, byt = 0;
    size_t max_len = 0, cur = 0;
    int in_word = 0;
    int ch;

    while ((ch = getchar()) != EOF) {
        unsigned char ub = (unsigned char)ch;
        byt++;

        if (ub == '\n') {
            nl++;
            if (cur > max_len)
                max_len = cur;
            cur = 0;
        } else {
            cur++;
            if (map[ub]) {
                /* space, \t, \v, \f: break word runs */
                in_word = 0;
            } else if (!in_word) {
                wrds++;
                in_word = 1;
            }
        }
    }

    printf("%ld %ld %ld %zu\n", nl, wrds, byt, max_len > cur ? max_len : cur);
    return 0;
}
