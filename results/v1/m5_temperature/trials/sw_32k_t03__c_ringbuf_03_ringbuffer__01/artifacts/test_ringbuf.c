#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int tests_run = 0;
static int tests_fail = 0;

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { printf("FAIL: %s\n", msg); tests_fail++; } \
    else { printf("PASS: %s\n", msg); } \
} while(0)

int main(void)
{
    /* rb_create / rb_destroy */
    ringbuf_t *rb = rb_create(8);
    CHECK(rb != NULL, "create returns non-NULL");
    CHECK(rb_capacity(rb) == 8, "capacity is 8");

    ringbuf_t *bad = rb_create(0);
    CHECK(bad == NULL, "create(0) returns NULL");

    /* rb_size on empty */
    CHECK(rb_size(rb) == 0, "empty size is 0");

    /* rb_write / rb_read basic */
    uint8_t wdata[] = {1,2,3,4,5};
    size_t n = rb_write(rb, wdata, 5);
    CHECK(n == 5, "write returns 5");
    CHECK(rb_size(rb) == 5, "size is 5 after write");

    uint8_t rdata[5];
    n = rb_read(rb, rdata, 5);
    CHECK(n == 5, "read returns 5");
    CHECK(memcmp(rdata, wdata, 5) == 0, "read matches written data");
    CHECK(rb_size(rb) == 0, "size is 0 after read");

    /* wrap-around write + read */
    uint8_t fill[] = {10,11,12,13,14,15,16,17};
    rb_write(rb, fill, 8);       /* fills buffer completely */
    CHECK(rb_size(rb) == 8, "size is 8 after full write");

    uint8_t discard[] = {99,99,99};
    rb_write(rb, discard, 3);    /* should store only 0 bytes (full) */
    CHECK(rb_size(rb) == 8, "size still 8 after write to full buffer");

    uint8_t out[8];
    n = rb_read(rb, out, 8);
    CHECK(n == 8 && memcmp(out, fill, 8) == 0, "read all 8 bytes correct");

    /* wrap-around: write past end */
    uint8_t a[] = {1,2,3,4};
    rb_write(rb, a, 4);          /* fills half */
    uint8_t b[] = {5,6,7,8,9,10,11,12};
    n = rb_write(rb, b, 8);      /* should write 4 (only 4 free) */
    CHECK(n == 4, "partial write returns 4");

    uint8_t result[8];
    rb_read(rb, result, 8);
    CHECK(result[0]==1 && result[3]==4 && result[4]==5 && result[7]==8, "wrap-around data correct");

    /* rb_peek — peek before consuming */
    uint8_t w2[] = {20,21,22,23};
    rb_write(rb, w2, 4);
    uint8_t peeked[4];
    n = rb_peek(rb, peeked, 4);
    CHECK(n == 4 && memcmp(peeked, w2, 4) == 0, "peek matches oldest bytes");
    CHECK(rb_size(rb) == 4, "size unchanged after peek");

    /* rb_clear */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear empties buffer");

    /* NULL safety */
    CHECK(rb_write(NULL, wdata, 5) == 0, "write(NULL) returns 0");
    CHECK(rb_read(NULL, rdata, 5) == 0, "read(NULL) returns 0");
    CHECK(rb_peek(NULL, peeked, 4) == 0, "peek(NULL) returns 0");
    CHECK(rb_size(NULL) == 0, "size(NULL) returns 0");
    CHECK(rb_capacity(NULL) == 0, "capacity(NULL) returns 0");
    rb_destroy(NULL);             /* should not crash */

    rb_destroy(rb);

    printf("\n%d/%d passed\n", tests_run - tests_fail, tests_run);
    return tests_fail > 0 ? 1 : 0;
}
