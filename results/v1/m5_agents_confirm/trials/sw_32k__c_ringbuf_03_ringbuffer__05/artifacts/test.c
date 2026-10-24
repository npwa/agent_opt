#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    ringbuf_t *rb;
    size_t n;
    uint8_t buf[32];
    uint8_t indata5[] = { 0x10, 0x11, 0x12, 0x13, 0x14 };

    rb = rb_create(16);
    if (!rb) { printf("FAIL: create\n"); return 1; }
    if (rb_capacity(rb) != 16) { printf("FAIL: capacity\n"); return 1; }
    if (rb_size(rb) != 0) { printf("FAIL: size empty\n"); return 1; }

    /* write, read exact match */
    n = rb_write(rb, indata5, 5);
    if (n != 5 || rb_size(rb) != 5) { printf("FAIL: write/size %zu/%u\n", n, (unsigned)rb_size(rb)); return 1; }

    {
        uint8_t out[5];
        size_t rn = rb_read(rb, out, 5);
        if (rn != 5 || memcmp(out, indata5, 5) != 0) { printf("FAIL: read\n"); return 1; }
        if (rb_size(rb) != 0) { printf("FAIL: empty after full read\n"); return 1; }
    }

    /* peek (non-destructive) */
    rb_write(rb, indata5, 5);
    {
        uint8_t out[3];
        n = rb_peek(rb, out, 3);
        if (n != 3 || memcmp(out, indata5, 3) != 0) { printf("FAIL: peek\n"); return 1; }
        if (rb_size(rb) != 5) { printf("FAIL: size after peek\n"); return 1; }
    }

    /* wrap-around: fill buffer, read some, write past end */
    rb_clear(rb);
    {
        uint8_t all255[16];
        memset(all255, 0xFF, 16);
        n = rb_write(rb, all255, 16);       /* full              */
        if (n != 16) { printf("FAIL: fill %zu\n", n); return 1; }

        uint8_t tmp[8];
        rb_read(rb, tmp, 8);                /* read first 8      */
        /* head=8, tail=0, filled=8                        */

        uint8_t newdat[8] = {0,1,2,3,4,5,6,7};
        n = rb_write(rb, newdat, 8);       /* wraps: tail 0→8   */
        if (n != 8) { printf("FAIL: wrap wr %zu\n", n); return 1; }

        memset(buf, 0, sizeof(buf));
        n = rb_read(rb, buf, 16);           /* should get 8xFF + 8 newdat */

        if (n != 16) { printf("FAIL: wrap rd %zu\n", n); return 1; }
        {
            size_t i;
            for (i = 0; i < 8; ++i) {
                if (buf[i] != 0xFFU) { printf("FAIL: wrap rdat[%zu]=%u i=%zu\n", i, buf[i], i); return 1; }
                if (buf[i+8] != i)      { printf("FAIL: newdat[%d]=%u exp=%d\n", (int)i, buf[i+8], (int)i); return 1; }
            }
        }
    }

    /* partial write */
    rb_clear(rb);
    {
        uint8_t big[40];
        memset(big, 0xCC, sizeof(big));
        n = rb_write(rb, big, 40);
        if (n != 16) { printf("FAIL: partial %zu\n", n); return 1; }
    }

    /* NULL safety */
    rb_destroy(NULL);
    if (rb_size(NULL) != 0 || rb_capacity(NULL) != 0) { printf("FAIL: null helpers\n"); return 1; }
    if (rb_create(0) != NULL) { printf("FAIL: create(0)\n"); return 1; }

    rb_destroy(rb);
    printf("All tests passed!\n");
    return 0;
}
