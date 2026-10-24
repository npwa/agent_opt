#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    /* Test NULL handling */
    if (rb_create(0) != NULL) { printf("FAIL: create capacity 0\n"); return 1; }
    rb_destroy(NULL);   /* should not crash */

    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FAIL: create\n"); return 1; }
    if (rb_capacity(rb) != 8) { printf("FAIL: capacity\n"); return 1; }
    if (rb_size(rb) != 0) { printf("FAIL: initial size\n"); return 1; }

    /* Write 5 bytes and verify */
    uint8_t wdata[] = {1,2,3,4,5};
    size_t n = rb_write(rb, wdata, 5);
    if (n != 5) { printf("FAIL: write returned %zu\n", n); return 1; }
    if (rb_size(rb) != 5) { printf("FAIL: size after write\n"); return 1; }

    /* Peek */
    uint8_t peek[5];
    if (rb_peek(rb, peek, 5) != 5) { printf("FAIL: peek count\n"); return 1; }
    for (int i = 0; i < 5; i++)
        if (peek[i] != wdata[i]) { printf("FAIL: peek data\n"); return 1; }

    /* Read back */
    uint8_t rdata[5];
    if (rb_read(rb, rdata, 5) != 5) { printf("FAIL: read\n"); return 1; }
    for (int i = 0; i < 5; i++)
        if (rdata[i] != wdata[i]) { printf("FAIL: read data\n"); return 1; }
    if (rb_size(rb) != 0) { printf("FAIL: size after read\n"); return 1; }

    /* Wrap-around test: capacity 8, write past the end */
    n = rb_write(rb, wdata, 5);
    uint8_t data2[] = {6,7,8,9,10,11};
    size_t m = rb_write(rb, data2, 6);
    if (m != 3) { printf("FAIL: wrap write returned %zu\n", m); return 1; }
    /* buffer now contains: wdata[0..4] then data2[0..2], i.e. 5+3=8 bytes full */

    uint8_t rbuf[8];
    if (rb_read(rb, rbuf, 8) != 8) { printf("FAIL: read wrap\n"); return 1; }
    for (int i = 0; i < 5; i++)
        if (rbuf[i] != wdata[i]) { printf("FAIL: wrap read data[%d]=%d\n", i, rbuf[i]); return 1; }
    for (int i = 0; i < 3; i++)
        if (rbuf[5+i] != data2[i]) { printf("FAIL: wrap read data2[%d]=%d\n", i, rbuf[5+i]); return 1; }

    /* Clear */
    rb_clear(rb);
    if (rb_size(rb) != 0) { printf("FAIL: clear\n"); return 1; }

    /* Write after clear should go from head=0 again */
    n = rb_write(rb, wdata, 5);
    uint8_t r2[5];
    rb_read(rb, r2, 5);
    for (int i = 0; i < 5; i++)
        if (r2[i] != wdata[i]) { printf("FAIL: write after clear\n"); return 1; }

    /* Partial write */
    uint8_t filler[] = {100, 101, 102};
    rb_write(rb, filler, 3);

    n = rb_write(rb, data2, 6);
    size_t full_after = rb_size(rb);
    if (n != 5) { printf("FAIL: partial write returned %zu expected 5\n", n); return 1; }
    if (full_after != 8) { printf("FAIL: partial write size after is %zu expect 8\n", full_after); return 1; }

    n = rb_read(rb, r2, 5);
    for (int i = 0; i < 3; i++)
        if (r2[i] != wdata[i]) { printf("FAIL: partial read data[%d]=%d\n", i, r2[i]); return 1; }
    for (int i = 3; i < 5; i++)
        if (r2[i] != data2[i-3]) { printf("FAIL: partial read overflow[%d]=%d\n", i, r2[i]); return 1; }

    rb_destroy(rb);

    printf("All tests passed.\n");
    return 0;
}
