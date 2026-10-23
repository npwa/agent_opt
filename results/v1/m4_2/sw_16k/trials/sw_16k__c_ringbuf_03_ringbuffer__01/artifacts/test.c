#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ringbuf.h"

static int passed = 0, failed = 0;

#define TEST(cond) do { \
    if (cond) { passed++; } \
    else { failed++; printf("FAIL: %s at line %d\n", #cond, __LINE__); } \
} while (0)

int main(void)
{
    ringbuf_t *rb;
    uint8_t buf[16];
    size_t n;

    /* NULL safety */
    TEST(rb_create(0) == NULL);
    rb_destroy(NULL);
    TEST(rb_read(NULL, buf, 1) == 0);
    TEST(rb_write(NULL, buf, 1) == 0);
    TEST(rb_peek(NULL, buf, 1) == 0);

    rb = rb_create(4);
    TEST(rb != NULL);
    TEST(rb_capacity(rb) == 4);
    TEST(rb_size(rb) == 0);

    /* Write fits */
    uint8_t w[4] = {1, 2, 3, 4};
    n = rb_write(rb, w, 4);
    TEST(n == 4);
    TEST(rb_size(rb) == 4);

    /* Peek (doesn't remove bytes) */
    n = rb_peek(rb, buf, 8);
    TEST(n == 4);
    TEST(memcmp(buf, w, 4) == 0);
    TEST(rb_size(rb) == 4); /* unchanged */

    /* Read all */
    memset(buf, 0, sizeof(buf));
    n = rb_read(rb, buf, 16);
    TEST(n == 4);
    TEST(memcmp(buf, w, 4) == 0);
    TEST(rb_size(rb) == 0);

    /* Partial write (write more than fits) */
    uint8_t big[8] = {10, 11, 12, 13, 14, 15, 16, 17};
    n = rb_write(rb, big, 8);
    TEST(n == 4);
    TEST(rb_size(rb) == 4);

    /* Verify: written bytes are the first 4 of big */
    uint8_t want[4] = {10, 11, 12, 13};
    n = rb_read(rb, buf, 8);
    TEST(n == 4);
    TEST(memcmp(buf, want, 4) == 0);

    /* Write 1, read 1, write fill to trigger wrap-around */
    rb_clear(rb);
    uint8_t a1[1] = {42};
    n = rb_write(rb, a1, 1);
    TEST(n == 1);
    TEST(rb_size(rb) == 1);

    /* Now write 3 more: should wrap around */
    uint8_t blk[3] = {99, 100, 101};
    n = rb_write(rb, blk, 3);
    TEST(n == 3);
    TEST(rb_size(rb) == 4);

    /* Read all and verify order: 42, 99, 100, 101 */
    uint8_t want2[4] = {42, 99, 100, 101};
    n = rb_read(rb, buf, 4);
    TEST(n == 4);
    TEST(memcmp(buf, want2, 4) == 0);

    /* Write 2, clear, write 3 (overwrites old head position) */
    rb_clear(rb);
    uint8_t c1[4] = {1, 2, 3, 4};
    n = rb_write(rb, c1, 4);
    TEST(n == 4);

    uint8_t rbuf[2];
    n = rb_read(rb, rbuf, 2);
    TEST(n == 2);
    TEST(rbuf[0] == 1 && rbuf[1] == 2);
    TEST(rb_size(rb) == 2);

    rb_clear(rb);
    uint8_t c2[3] = {5, 6, 7};
    n = rb_write(rb, c2, 3);
    TEST(n == 3);
    n = rb_read(rb, buf, 4);
    TEST(n == 3);
    TEST(buf[0]==5 && buf[1]==6 && buf[2]==7);

    printf("%d passed, %d failed\n", passed, failed);
    rb_destroy(rb);
    return failed > 0 ? 1 : 0;
}
