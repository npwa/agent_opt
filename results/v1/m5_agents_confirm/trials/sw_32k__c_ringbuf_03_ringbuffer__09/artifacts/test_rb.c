#include <stdio.h>
#include "string.h"
#include "ringbuf.h"

int main(void)
{
    int ok = 1;
    ringbuf_t *rb;

    rb = rb_create(8);
    if (!rb || rb_capacity(rb) != 8 || rb_size(rb) != 0) { puts("FAIL create/capacity/size"); ok = 0; }

    uint8_t inp[] = {1,2,3,4,5};
    if (rb_write(rb, inp, 5) != 5 || rb_size(rb) != 5) { puts("FAIL write"); ok = 0; }

    uint8_t out[5] = {0};
    if (rb_read(rb, out, 10) != 5) { puts("FAIL read all"); ok = 0; }
    for (int i = 0; i < 5; i++) { if (out[i] != (uint8_t)(i+1)) { puts("FAIL round-trip"); ok = 0; break; } }

    /* wrap-around write */
    rb_clear(rb);
    uint8_t blk[] = "hello world";
    size_t w = rb_write(rb, blk, 12);
    if (w != 8 || rb_size(rb) != 8) { puts("FAIL capacity clamping"); ok = 0; }

    /* peek should return first 4 bytes written */

    uint8_t peeked4[4] = {0};
    size_t p = rb_peek(rb, peeked4, 4);
    if (p != 4) { puts("FAIL peek count"); ok = 0; } else if (!memcmp(peeked4, "hell", 4)) { puts("FAIL peek data"); ok = 0; }

    /* read the remaining 8 bytes */
    uint8_t buf8[16] = {0};
    if (rb_read(rb, buf8, 8) != 8) { puts("FAIL read all"); ok = 0; }
    if (buf8[0] != 'h' && buf8[7] != 'r') { puts("FAIL peeked buffer corrupt"); ok = 0; }

    /* write in multiple chunks */
    rb_clear(rb);
    uint8_t first[] = "abc";
    uint8_t second[] = "defghi";
    size_t w1 = rb_write(rb, first, 3);
    size_t w2 = rb_write(rb, second, 6);
    if (w1 != 3 || w2 != 5 || rb_size(rb) != 8) { puts("FAIL partial write"); ok = 0; }

    uint8_t verify[8] = {0};
    size_t r = rb_read(rb, verify, 8);
    if (r != 8 || verify[0] != 'a' || verify[7] != 'd') { puts("FAIL wrap order"); ok = 0; }

    /* NULL safety */
    rb_destroy(NULL);
    if (rb_write(NULL, inp, 1) != 0 || rb_read(NULL, out, 1) != 0) { puts("FAIL NULL write/read"); ok = 0; }
    if (rb_size(NULL) != 0 || rb_capacity(NULL) != 0) { puts("FAIL NULL size/capacity"); ok = 0; }

    if (rb_create(0) != NULL) { puts("FAIL zero create"); ok = 0; }

    rb_destroy(rb);

    puts(ok ? "ALL OK" : "FAIL");
    return ok ? 0 : 1;
}
