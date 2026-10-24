#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

#define CHK(expr, msg)                         \
    do {                                         \
        if (!(expr)) {                           \
            printf("FAIL: %s (line %d)\n", msg, __LINE__); \
            failed = 1;                          \
        }                                        \
    } while (0)

int main(void)
{
    int failed = 0;

    /* ---- rb_create / rb_destroy NULL handling ---- */
    CHK(rb_create(0) == NULL, "rb_create(0)");
    CHK(rb_capacity(NULL) == 0, "rb_capacity(NULL)");
    CHK(rb_size(NULL) == 0, "rb_size(NULL)");
    rb_destroy(NULL);   /* should not crash */

    /* ---- basic lifecycle ---- */
    ringbuf_t *rb = rb_create(4);
    CHK(rb != NULL, "rb_create(4)");
    CHK(rb_capacity(rb) == 4, "capacity is 4");
    CHK(rb_size(rb)   == 0, "initial size is 0");

    /* ---- write a few bytes ---- */
    {
        const uint8_t data[4] = {1, 2, 3, 4};
        size_t n = rb_write(rb, data, 4);
        CHK(n == 4, "wrote all 4 bytes");
        CHK(rb_size(rb) == 4, "size is 4 after write");
    }

    /* ---- read them out in order (FIFO) ---- */
    {
        uint8_t buf[4];
        size_t n = rb_read(rb, buf, 4);
        CHK(n == 4, "read all 4 bytes");
        CHK(memcmp(buf, "\x01\x02\x03\x04", 4) == 0, "bytes in FIFO order");
    }

    /* ---- buffer should be empty now ---- */
    CHK(rb_size(rb) == 0, "size is 0 after full read");
    {
        uint8_t tmp;
        CHK(rb_read(rb, &tmp, 1) == 0, "read from empty returns 0");
    }

    /* ---- partial write (write larger than free space) ---- */
    rb_clear(rb);
    {
        const uint8_t big[256];
        memset((void *)big, 0xAA, sizeof(big));
        size_t n = rb_write(rb, big, sizeof(big));
        CHK(n == 4, "write larger than capacity stores only what fits");
        CHK(rb_size(rb) == 4, "size is capacity after partial write");
    }

    /* ---- wrap-around write + read ---- */
    rb_clear(rb);
    {
        const uint8_t a[3] = {10, 20, 30};
        size_t n;

        n = rb_write(rb, a, 3);
        CHK(n == 3, "wrote 3");

        /* overflow the buffer */
        const uint8_t b[4] = {40, 50, 60, 70};
        n = rb_write(rb, b, 4);   /* only 1 byte fits */
        CHK(n == 1, "partial write (overflow) stores 1 byte");

        /* read — buffer now has [10,20,30,40], all 4 readable */
        uint8_t out[4];
        n = rb_read(rb, out, 4);
        CHK(n == 4, "read all 4 bytes after overflow");
        CHK(out[0] == 10 && out[1] == 20 && out[2] == 30 && out[3] == 40, "overflow data intact");
    }

    /* ---- peek does not remove data ---- */
    rb_clear(rb);
    {
        const uint8_t d[4] = {99, 88, 77, 66};
        rb_write(rb, d, 4);
        {
            uint8_t p[2];
            size_t n = rb_peek(rb, p, 2);
            CHK(n == 2, "peek returned count");
            CHK(p[0] == 99 && p[1] == 88, "peek returns oldest bytes");
        }
        CHK(rb_size(rb) == 4, "size unchanged after peek");
    }

    /* ---- clear then write/peek/read NULL args all return 0 ---- */
    rb_clear(rb);
    {
        uint8_t tmp;
        const uint8_t *cd = (const uint8_t *)"\x01";
        CHK(rb_write(NULL, cd, 1) == 0, "write NULL rb");
        CHK(rb_read(NULL, &tmp, 1) == 0, "read NULL rb");
        CHK(rb_peek(NULL, &tmp, 1) == 0, "peek NULL rb");
        CHK(rb_write(rb, NULL, 1) == 0, "write NULL data");
        CHK(rb_read(rb, NULL, 1) == 0, "read NULL out");
        CHK(rb_peek(rb, NULL, 1) == 0, "peek NULL out");
    }

    /* ---- full cycle on multiple small round-trips ---- */
    rb_clear(rb);
    for (int round = 0; round < 100; round++) {
        const uint8_t w[4] = {(uint8_t)round, (uint8_t)(round+1),
                              (uint8_t)(round+2), (uint8_t)(round+3)};
        uint8_t r[4];
        rb_write(rb, w, 4);
        size_t nr = rb_read(rb, r, 4);
        CHK(nr == 4 && memcmp(w, r, 4) == 0, "round-trip FIFO data correct");
    }

    rb_destroy(rb);

    if (failed)
        printf("\n*** %d test(s) FAILED ***\n", failed);
    else
        printf("All tests passed.\n");

    return failed ? 1 : 0;
}
