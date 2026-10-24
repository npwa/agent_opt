#include <stdio.h>
#include <string.h>
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
    printf("after write 12 into cap8: w=%zu size=%zu head=%zu tail=%zu\n", 
           w, rb_size(rb), /* rb->head, rb->tail not accessible */
           rb_size(rb));
    if (w != 8 || rb_size(rb) != 8) { puts("FAIL capacity clamping"); ok = 0; }

    uint8_t peeked4[8] = {0};
    size_t p = rb_peek(rb, peeked4, 8);
    printf("peaked: \"");
    for (size_t i=0;i<p;i++) putchar(peeked4[i]);
    puts("\"");
    if (p != 8 || peeked4[0] != 'h' || peeked4[1] != 'e' || peeked4[2] != 'l' || peeked4[3] != 'l') { puts("FAIL peek data"); ok = 0; }

    read(rb, out, 8);
    printf("read back: \"");
    for (size_t i=0;i<8;i++) putchar(out[i]);
    puts("\"");

    rb_destroy(rb);
    puts(ok ? "ALL OK" : "FAIL");
    return ok ? 0 : 1;
}
