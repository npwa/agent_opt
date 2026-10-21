#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int ok = 1;
static void check(int cond, const char *name)
{
    if (cond) { printf("  PASS: %s\n", name); }
    else      { printf("  FAIL: %s\n", name); ok = 0; }
}

int main(void)
{
    /*--- NULL / edge cases ---*/
    puts("[edge]");
    rb_destroy(NULL);                       /* must not crash */
    check(rb_create(0) == NULL, "capacity 0 rejects");
    check(rb_write(NULL, NULL, 1) == 0,   "rb_write NULL returns 0");
    check(rb_read(NULL, NULL, 1)  == 0,    "rb_read  NULL returns 0");
    check(rb_peek(NULL, NULL, 1)  == 0,    "rb_peek  NULL returns 0");
    check(rb_size(NULL)       == 0,         "rb_size  NULL returns 0");
    check(rb_capacity(NULL)   == 0,         "rb_capac NULL returns 0");
    rb_clear(NULL);                           /* must not crash */

    /*--- create / destroy ---*/
    ringbuf_t *rb = rb_create(4);
    check(rb != NULL,                 "create returns non-NULL");
    check(rb_size(rb)      == 0,      "initial size == 0");
    check(rb_capacity(rb)  == 4,      "capacity == 4");

    /*--- basic write / read ---*/
    puts("[wrap]");
    uint8_t in[4]  = {10, 20, 30, 40};
    uint8_t out[4];
    size_t n;

    n = rb_write(rb, in, 4);
    check(n == 4 && rb_size(rb) == 4, "write 4 bytes fills buffer");
    n = rb_write(rb, in, 1);          /* full — nothing fits */
    check(n == 0,                     "write on full returns 0");

    n = rb_read(rb, out, 2);
    check(n == 2 && memcmp(out, "\x0a\x14", 2) == 0, "read 2 oldest");
    check(rb_size(rb) == 2,           "size after partial read == 2");

    /*--- wrap-around write then read ---*/
    uint8_t in2[3] = {50, 60, 70};
    n = rb_write(rb, in2, 3);         /* only 2 fit (cap=4, held=2) */
    check(n == 2,                     "partial write stores what fits");

    uint8_t out2[4];
    n = rb_read(rb, out2, 10);        /* request exceeds capacity */
    check(n == 4 && memcmp(out2, "\x1e\x28\x32\x3c", 4) == 0, "read all after wrap");
    check(rb_size(rb) == 0,           "buffer empty again");

    /*--- peek ---*/
    rb_write(rb, in, 3);               /* fill {10,20,30} */
    uint8_t pk[2];
    n = rb_peek(rb, pk, 2);
    check(n == 2 && memcmp(pk, "\x0a\x14", 2) == 0, "peek oldest two");
    check(rb_size(rb) == 3,           "peek does not change size");

    /*--- clear ---*/
    rb_clear(rb);
    check(rb_size(rb) == 0 && rb_capacity(rb) == 4, "clear empties but keeps capacity");

    /*--- destroy NULL ---*/
    rb_destroy(rb);
    rb_destroy(NULL);                   /* must not crash */

    puts(ok ? "\nAll tests passed." : "\nSome tests FAILED.");
    return ok ? 0 : 1;
}
