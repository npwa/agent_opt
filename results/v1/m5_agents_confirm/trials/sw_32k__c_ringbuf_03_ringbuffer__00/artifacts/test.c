#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int pass = 1;

    /* rb_create NULL / capacity == 0 */
    if (rb_create(0) != NULL) { pass = 0; printf("FAIL: create(0)\n"); }

    /* basic create/clear/destroy */
    ringbuf_t *rb = rb_create(8);
    if (!rb)   { pass = 0; printf("FAIL: create(8)\n"); }
    if (rb_capacity(rb) != 8) { pass = 0; printf("FAIL: capacity\n"); }
    if (rb_size(rb)     != 0) { pass = 0; printf("FAIL: size after create\n"); }

    /* write & read round-trip */
    uint8_t wr[] = {1,2,3,4,5};
    size_t w = rb_write(rb, wr, sizeof(wr));
    if (w != 5) { pass = 0; printf("FAIL: wrote %zu (expected 5)\n", w); }
    if (rb_size(rb) != 5) { pass = 0; printf("FAIL: size after write\n"); }

    uint8_t rd[5];
    size_t r = rb_read(rb, rd, sizeof(rd));
    if (r != 5 || memcmp(rd, wr, 5) != 0) { pass = 0; printf("FAIL: read\n"); }

    /* wrap-around test: fill partially, write past end */
    rb_clear(rb);
    uint8_t a[] = {10,11,12};
    rb_write(rb, a, sizeof(a));
    size_t remaining = rb_capacity(rb) - rb_size(rb);
    uint8_t b[64];
    memset(b, 20, sizeof(b));
    rb_write(rb, b, remaining + 1); /* one byte should go past end and wrap */

    /* destroy NULL is safe */
    rb_destroy(NULL);

    /* peek test */
    ringbuf_t *rb2 = rb_create(4);
    uint8_t ptest[] = {99};
    uint8_t peek_buf[1];
    rb_write(rb2, ptest, 1);
    size_t pn = rb_peek(rb2, peek_buf, 1);
    if (pn != 1 || peek_buf[0] != 99) { pass = 0; printf("FAIL: peek\n"); }

    /* write > free-space returns what fits */
    rb_clear(rb);
    uint8_t big[] = {1,2,3,4,5};
    (void)rb_write(rb, big, sizeof(big));
    remaining = 8 - 5;
    size_t wf2 = rb_write(rb, big, sizeof(big));
    if (wf2 != remaining) { pass = 0; printf("FAIL: partial write %zu expected %zu\n", wf2, remaining); }

    /* clear test */
    rb_clear(rb);
    if (rb_size(rb) != 0) { pass = 0; printf("FAIL: clear\n"); }

    /* wrap-around verification: read should return oldest first */
    rb_clear(rb);
    uint8_t pre[] = {1,2};
    rb_write(rb, pre, 2);
    size_t rem = rb_capacity(rb) - rb_size(rb);
    uint8_t mid[64];
    memset(mid, 0x5A, rem);       /* fill to capacity */
    rb_write(rb, mid, rem);
    uint8_t post[] = {3,4};         /* these wrap around */
    rb_write(rb, post, 2);
    uint8_t result[8];
    size_t got = rb_read(rb, result, sizeof(result));
    if (got != 8 || result[0] != 1 || result[1] != 2 || result[6] != 3 || result[7] != 4) {
        pass = 0;
        printf("FAIL: wrap-around read\n");
    }

    if (!pass) { printf("\nFAILED!\n"); return 1; }
    printf("All tests passed.\n");
    rb_destroy(rb);
    rb_destroy(rb2);
    return 0;
}
