#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int err = 0;

    /* Test rb_create / rb_capacity */
    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FAIL: create\n"); return 1; }
    if (rb_capacity(rb) != 8) { printf("FAIL: capacity\n"); err = 1; }

    /* Test NULL handling */
    if (rb_size(NULL) != 0) { printf("FAIL: size null\n"); err = 1; }
    if (rb_write(NULL, 0, 0) != 0) { printf("FAIL: write null\n"); err = 1; }
    if (rb_read(NULL, 0, 0) != 0) { printf("FAIL: read null\n"); err = 1; }
    rb_destroy(NULL); /* should not crash */

    /* Test write and read on empty/full */
    uint8_t out[4];
    if (rb_read(rb, out, 4) != 0) { printf("FAIL: read empty\n"); err = 1; }

    const uint8_t w[] = {1,2,3,4,5};
    size_t n = rb_write(rb, w, 5);
    if (n != 5) { printf("FAIL: write %zu expected 5\n", n); err = 1; }

    if (rb_size(rb) != 5) { printf("FAIL: size after write\n"); err = 1; }

    /* Fill remaining */
    const uint8_t w2[] = {6,7,8};
    rb_write(rb, w2, 3); /* only fits some */
    if (rb_size(rb) != 8) { printf("FAIL: size fill\n"); err = 1; }

    /* Write more when full */
    n = rb_write(rb, w2, 3);
    if (n != 0) { printf("FAIL: write full got %zu\n", n); err = 1; }

    /* Drain one and write again */
    uint8_t d[4];
    rb_read(rb, d, 1);
    n = rb_write(rb, w2, 3);
    if (n != 1) { printf("FAIL: write after drain got %zu\n", n); err = 1; }

    /* Check FIFO order */
    if (rb_size(rb) != 8) { printf("FAIL: size 2\n"); err = 1; }
    while (rb_size(rb) > 0) {
        uint8_t b;
        rb_read(rb, &b, 1);
        /* order: data written minus the one byte consumed earlier */
    }

    /* Test wrap-around with small buffer */
    ringbuf_t *small = rb_create(4);
    rb_write(small, (const uint8_t[]){0x11, 0x22}, 2);
    rb_read(small, out, 1);       /* drain 1, tail=1 */
    rb_read(small, out, 1);       /* drain 1, tail=2 */
    /* head would be at (2+2)%4=0 after wrap */
    uint8_t w3[] = {0xAA, 0xBB, 0xCC, 0xDD};
    rb_write(small, w3, 4);       /* should fill all with wrap */
    if (rb_size(small) != 4) { printf("FAIL: wrap size\n"); err = 1; }

    /* Peek test */
    ringbuf_t *peek = rb_create(8);
    uint8_t peek_data[] = {9,10,11};
    rb_write(peek, peek_data, 3);
    uint8_t p[3];
    size_t pn = rb_peek(peek, p, 3);
    if (pn != 3) { printf("FAIL: peek count %zu\n", pn); err = 1; }
    if (p[0] != 9 || p[1] != 10 || p[2] != 11) { printf("FAIL: peek data\n"); err = 1; }
    if (rb_size(peek) != 3) { printf("FAIL: peek didn't change size\n"); err = 1; }

    /* Clear test */
    rb_clear(peek);
    if (rb_size(peek) != 0) { printf("FAIL: clear\n"); err = 1; }

    /* Large write that exceeds capacity should partially succeed */
    ringbuf_t *part = rb_create(4);
    uint8_t big[] = {1,2,3,4,5};
    n = rb_write(part, big, 5);
    if (n != 4) { printf("FAIL: partial write %zu\n", n); err = 1; }

    /* Test wrap-around read correctness */
    uint8_t r[4];
    size_t rn = rb_read(part, r, 4);
    if (rn != 4 || r[0] != 1 || r[1] != 2 || r[2] != 3 || r[3] != 4) {
        printf("FAIL: wrap read %zu (%d,%d,%d,%d)\n", rn, r[0],r[1],r[2],r[3]);
        err = 1;
    }

    if (err) {
        printf("\nSOME TESTS FAILED\n");
        return 1;
    }
    printf("All tests passed.\n");
    rb_destroy(part);
    rb_destroy(peek);
    rb_destroy(small);
    return 0;
}
