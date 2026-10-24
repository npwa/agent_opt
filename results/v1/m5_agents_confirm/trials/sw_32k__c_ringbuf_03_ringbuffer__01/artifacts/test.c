#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int passed = 0;
static int failed = 0;

#define check(cond, msg) do { \
    if (cond) { passed++; printf("  PASS: %s\n", msg); } \
    else      { failed++; printf(" FAIL: %s\n", msg); } \
} while(0)

static void test_basic(void)
{
    ringbuf_t *rb = rb_create(8);

    check(rb != NULL, "create returns non-NULL");
    check(rb_capacity(rb) == 8,       "capacity is 8");
    check(rb_size(rb) == 0,           "size starts at 0");

    uint8_t data[] = {1,2,3,4,5};
    size_t w = rb_write(rb, data, sizeof(data));
    check(w == 5, "write writes all 5 bytes");
    check(rb_size(rb) == 5, "size is 5 after write");

    uint8_t rd[5];
    size_t r = rb_read(rb, rd, sizeof(rd));
    check(r == 5, "read reads all 5 bytes");
    check(memcmp(rd, data, 5) == 0, "read returns correct data");
    check(rb_size(rb) == 0, "size is 0 after read");

    rb_destroy(rb);
}

static void test_wrap_around(void)
{
    ringbuf_t *rb = rb_create(4);

    uint8_t a[] = {1,2,3};
    check(rb_write(rb, a, 3) == 3, "write 3 of 4 capacity");
    check(rb_size(rb) == 3, "size after write 3");

    uint8_t rd[2];
    check(rb_read(rb, rd, 2) == 2, "read 2 bytes");
    check(rd[0] == 1 && rd[1] == 2, "first two bytes match");
    check(rb_size(rb) == 1, "size is 1 after partial read");

    uint8_t b[] = {99, 100};   /* will write to buffer, which has space for 5 but capacity limits to 2 */
    size_t w = rb_write(rb, b, 2);
    check(w == 2, "write 2 more (wraps)");

    uint8_t out[3];
    check(rb_read(rb, out, 3) == 3, "read all remaining");
    check(out[0] == 3 && out[1] == 99 && out[2] == 100, "data correct after wrap-around");

    rb_destroy(rb);
}

static void test_fill_up(void)
{
    ringbuf_t *rb = rb_create(2);

    uint8_t a[] = {10};
    check(rb_write(rb, a, 1) == 1, "write 1 of 2");

    uint8_t b[] = {30, 40, 50};
    size_t w = rb_write(rb, b, sizeof(b));
    check(w == 1, "partial write: only 1 byte fits (capacity=2)");

    rb_destroy(rb);
}

static void test_peek(void)
{
    ringbuf_t *rb = rb_create(4);

    uint8_t data[] = {10, 20, 30};
    rb_write(rb, data, sizeof(data));

    uint8_t out[3];
    check(rb_peek(rb, out, sizeof(out)) == 3, "peek reads correct count");
    check(out[0] == 10 && out[1] == 20 && out[2] == 30, "peek returns correct data");

    check(rb_size(rb) == 3, "size unchanged after peek");

    rb_destroy(rb);
}

static void test_clear(void)
{
    ringbuf_t *rb = rb_create(4);

    uint8_t data[] = {5,6,7};
    rb_write(rb, data, sizeof(data));
    check(rb_size(rb) == 3, "size is 3 before clear");

    rb_clear(rb);
    check(rb_size(rb) == 0, "size is 0 after clear");

    uint8_t out[4];
    check(rb_read(rb, out, sizeof(out)) == 0, "read on empty returns 0");

    rb_destroy(rb);
}

static void test_null_safety(void)
{
    check(rb_create(0) == NULL, "create with cap=0 returns NULL");
    check(rb_create(SIZE_MAX) == NULL, "create with cap=SIZE_MAX returns NULL");
    check(rb_write(NULL, NULL, 10) == 0, "write NULL is 0");
    check(rb_read(NULL, NULL, 10) == 0, "read NULL is 0");
    check(rb_peek(NULL, NULL, 10) == 0, "peek NULL is 0");
    check(rb_size(NULL) == 0,   "size NULL is 0");
    check(rb_capacity(NULL) == 0,"capacity NULL is 0");
    rb_destroy(NULL); /* must not crash */
}

/* test_fill_up: capacity=2 so write(3 bytes) can only store 1 byte more (was already at 1/2). 
   total written = min(3, 2-1) = 1. */

static void test_partial_write(void)
{
    ringbuf_t *rb = rb_create(2);

    uint8_t data[] = {10, 20};
    check(rb_write(rb, data, sizeof(data)) == 2, "write full to capacity");
    check(rb_size(rb) == 2, "size is 2 (full)");

    /* now buffer is full; write more */
    uint8_t extra[] = {99, 100};
    size_t w = rb_write(rb, extra, sizeof(extra));
    check(w == 0, "write on full returns 0");

    check(rb_size(rb) == 2, "size still 2 after write-on-full");

    uint8_t out[2];
    check(rb_read(rb, out, sizeof(out)) == 2, "read all from full buffer");
    check(out[0] == 10 && out[1] == 20, "data is 10, 20");

    rb_destroy(rb);
}

int main(void)
{
    printf("=== basic test ===\n");   test_basic();
    printf("=== wrap-around test ===\n"); test_wrap_around();
    printf("=== fill up test ===\n");     test_fill_up();
    printf("=== peek test ===\n");        test_peek();
    printf("=== clear test ===\n");       test_clear();
    printf("=== null safety test ===\n"); test_null_safety();
    printf("=== partial write test ===\n");  test_partial_write();

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
