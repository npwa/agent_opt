/* Hidden verifier for coding_03_compress_gen: includes the agent's compress.c. */
#include <stdio.h>
#include <string.h>

#include "compress.c"

static int check(const char *in, const char *want) {
    char out[256];
    memset(out, 'X', sizeof out);
    compress_string(in, out);
    if (strcmp(out, want) != 0) {
        fprintf(stderr, "FAIL: \"%s\" -> \"%s\" (want \"%s\")\n", in, out, want);
        return 1;
    }
    return 0;
}

int main(void) {
    int bad = 0;
    bad += check("aabcccccaaa", "a2b1c5a3");
    bad += check("aabccc", "a2b1c3");
    bad += check("", "");
    bad += check("a", "a1");
    bad += check("abc", "a1b1c1");
    bad += check("AAaa", "A2a2");
    bad += check("zzzzzzzzzzzz", "z12");
    return bad ? 1 : 0;
}
