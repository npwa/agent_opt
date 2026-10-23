#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ringbuf.h"

#define TEST(name) printf("=== %s ===\n", name)
#define CHK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); return 1; } \
} while(0)

int main(void)
{
    TEST("rb_create / rb_capacity / rb_size");
    {
        ringbuf_t *rb = rb_create(16);
        CHK(rb != NULL, "create returned NULL");
        CHK(rb_capacity(rb) == 16, "capacity mismatch");
        CHK(rb_size(rb) == 0, "initial size should be 0");
        rb_destroy(rb);
    }

    TEST("rb_create invalid");
    {
        CHK(rb_create(0) == NULL, "zero capacity should fail");
    }

    TEST("rb_write / rb_read identity");
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t in[]  = {0x01, 0xAA, 0xFE, 0xFF};
        uint8_t out[8];
        size_t w = rb_write(rb, in, sizeof(in));
        CHK(w == sizeof(in), "write count mismatch");
        CHK(rb_size(rb) == sizeof(in), "size after write incorrect");
        size_t r = rb_read(rb, out, sizeof(out));
        CHK(r == sizeof(in), "read count mismatch for small buffer");
        CHK(memcmp(out, in, sizeof(in)) == 0, "data round-trip failed");
        CHK(rb_size(rb) == 0, "size after read should be 0");
        rb_destroy(rb);
    }

    TEST("rb_write partial (not enough free space)");
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t a[] = {1, 2};
        uint8_t b[] = {3, 4, 5, 6, 7};
        rb_write(rb, a, sizeof(a));
        size_t w = rb_write(rb, b, sizeof(b));
        CHK(w == 2, "partial write should return bytes that fitted");
        CHK(rb_size(rb) == 4, "full buffer size wrong");
        uint8_t out[5];
        size_t r = rb_read(rb, out, 5);
        CHK(r == 4, "read from full+partial buffer incorrect");
        CHK(out[0]==1 && out[3]==3 && out[4]==7, "FIFO order wrong after partial write");
        rb_destroy(rb);
    }

    TEST("wrap-around read/write");
    {
        ringbuf_t *rb = rb_create(6);
        /* fill and drain so tail wraps */
        uint8_t tmp[12];
        for (int i = 0; i < 12; i++) tmp[i] = (uint8_t)(i + 1);
        size_t w1 = rb_write(rb, tmp, 4);
        CHK(w1 == 4, "first write");
        uint8_t drain[3];
        size_t r1 = rb_read(rb, drain, 3);
        CHK(r1 == 3, "drain before wrap");

        /* now head=4 tail=3 (wrapped boundary at index 6) */
        uint8_t after[]  = {10, 20, 30, 40};
        size_t w2 = rb_write(rb, after, sizeof(after));
        CHK(w2 == 3, "write crossing wrap boundary should partial");

        /* read back: tail was at 3 */
        uint8_t rd[6];
        size_t r2 = rb_read(rb, rd, 6);
        CHK(r2 > 0, "should have data after wrap write");
        CHK(rd[r2-1] == (rb != NULL ? 2 : -1),  /* last byte depends on what fit */
            "wrap-around read back wrong");
        rb_destroy(rb);
    }

    TEST("rb_peek preserves buffer state");
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t in[] = {10, 20, 30};
        rb_write(rb, in, sizeof(in));
        uint8_t peeked[3];
        size_t n = rb_peek(rb, peeked, sizeof(peeked));
        CHK(n == 3, "peek returned wrong count");
        CHK(peeked[0]==10 && peeked[2]==30, "peek data mismatch");

        /* Size and content must still be intact */
        CHK(rb_size(rb) == 3, "peek mutated size");
        uint8_t out[3];
        size_t r = rb_read(rb, out, sizeof(out));
        CHK(r == 3, "read after peek should work normally");
        rb_destroy(rb);
    }

    TEST("rb_clear resets state");
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t in[] = {99};
        rb_write(rb, in, 1);
        CHK(rb_size(rb) == 1, "size before clear");
        rb_clear(rb);
        CHK(rb_size(rb) == 0, "size after clear wrong");
        /* write again over the same data */
        uint8_t second[] = {77};
        size_t w = rb_write(rb, second, sizeof(second));
        CHK(w == 1, "write after clear should succeed");
        uint8_t out[2];
        size_t r = rb_read(rb, out, sizeof(out));
        CHK(r == 1 && out[0] == 77, "data from cleared-then-written buffer wrong");
        rb_destroy(rb);
    }

    TEST("rb_{size,capacity,null} guards");
    {
        CHK(rb_size(NULL) == 0, "NULL rb_size should return 0");
        CHK(rb_capacity(NULL) == 0, "NULL rb_capsity should return 0");
        CHK(rb_write(NULL, (uint8_t[]){1}, 1) == 0, "write to NULL");
        CHK(rb_read(NULL, (uint8_t[1]){0}, 1) == 0, "read from NULL");
        CHK(rb_peek(NULL, (uint8_t[1]){0}, 1) == 0, "peek NULL");
        rb_destroy(NULL);   /* must not crash */
    }

    TEST("large data fills then partial writes then reads all");
    {
        ringbuf_t *rb = rb_create(2);
        uint8_t vals[] = {10, 20, 30, 40, 50, 60};
        for (int i = 0; i < (int)sizeof(vals); i++) {
            size_t w = rb_write(rb, &vals[i], 1);
            CHK(w == 1, "each individual write must fully fit");
            /* when full, next write will fail to write any */
            size_t fw = rb_write(rb, &vals[i], 10);
            if (fw != 0) { fprintf(stderr, "partial write count %zu at i=%d\n", fw, i); }
        }

        /* Should contain last 2 values that fit: [50, 60] with FIFO order */
        uint8_t rd[3];
        size_t r = rb_read(rb, rd, sizeof(rd));
        CHK(r == 2, "should read exactly what's in a capacity-2 buffer");

        /* The FIFO history: write 10(overflows), 20(overflows) ... 
           Actually each writes 1 then tries to write 10 more. */
        rb_destroy(rb);
    }

    printf("\nAll tests passed.\n");
    return 0;
}
