#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int passed = 0, failed = 0;

#define CHECK(cond, msg) do {                        \
    if (cond) { ++passed; printf("  PASS: %s\n", msg); } \
    else { ++failed; printf("  FAIL: %s\n", msg); }   \
} while(0)

void test_basic_create_destroy(void) {
    printf("--- basic create / destroy ---\n");
    ringbuf_t *rb = rb_create(16);
    CHECK(rb != NULL, "create returns non-NULL");
    CHECK(rb_capacity(rb) == 16, "capacity == 16");
    CHECK(rb_size(rb) == 0, "size == 0 initially");

    rb_destroy(rb);        /* normal destroy */
    rb_destroy(NULL);      /* NULL destroy must not crash */
}

void test_write_read(void) {
    printf("--- write / read ---\n");
    ringbuf_t *rb = rb_create(4);
    uint8_t in[]  = {1,2,3,4,5};
    uint8_t out[8];

    /* fills exactly */
    size_t n = rb_write(rb, in, 4);
    CHECK(n == 4, "write 4 bytes returns 4");
    CHECK(rb_size(rb) == 4, "size == 4 after write");

    /* empty buffer can't grow */
    n = rb_write(rb, in+4, 1);
    CHECK(n == 0, "write on full returns 0");

    /* read back */
    n = rb_read(rb, out, 5);          /* ask more than available */
    CHECK(n == 4, "read 5 returns 4");
    CHECK(memcmp(out, in, 4) == 0, "data matches FIFO order");
    CHECK(rb_size(rb) == 0, "size == 0 after read");

    /* read on empty */
    n = rb_read(rb, out, 1);
    CHECK(n == 0, "read on empty returns 0");

    rb_destroy(rb);
}

void test_wraparound_write(void) {
    printf("--- wrap-around write ---\n");
    ringbuf_t *rb = rb_create(4);

    /* fill */
    uint8_t a[] = {1,2,3,4};
    rb_write(rb, a, 4);
    /* drain partially */
    uint8_t tmp[2];
    rb_read(rb, tmp, 2);   /* head == 2 */

    /* now write one more -> must wrap to index 0 */
    uint8_t b[] = {5};
    size_t n = rb_write(rb, b, 1);
    CHECK(n == 1, "write after partial drain returns 1");
    CHECK(rb_size(rb) == 3, "size == 3");

    /* read all, expect 2,3,5 */
    uint8_t out[4];
    rb_read(rb, out, 6);
    CHECK(out[0]==3 && out[1]==4 && out[2]==5, "wrap-around FIFO correct");

    rb_destroy(rb);
}

void test_wraparound_peek(void) {
    printf("--- peek / read interleaved ---\n");
    ringbuf_t *rb = rb_create(4);
    uint8_t a[]  = {10,20};
    uint8_t b[]  = {30,40};
    uint8_t peek[6];

    rb_write(rb, a, 2);
    size_t n = rb_peek(rb, peek, 3);
    CHECK(n == 2, "peek returns count available");
    CHECK(peek[0]==10 && peek[1]==20, "peek data matches");

    /* peek does NOT remove */
    CHECK(rb_size(rb) == 2, "size unchanged after peek");

    rb_write(rb, b, 2);   /* fill */
    uint8_t out[4];
    rb_read(rb, out, 4);
    CHECK(out[0]==10 && out[3]==40, "full FIFO preserved");

    rb_destroy(rb);
}

void test_partial_write(void) {
    printf("--- partial write (spec requirement) ---\n");
    ringbuf_t *rb = rb_create(2);

    uint8_t buf[] = {1,2,3,4};
    size_t n = rb_write(rb, buf, 4);   /* len > free space */
    CHECK(n == 2, "write larger than free returns fit count");

    uint8_t out[2];
    rb_read(rb, out, 2);
    CHECK(out[0]==1 && out[1]==2, "partial write stored correct bytes");

    /* drain then write more */
    n = rb_write(rb, buf+2, 4);         /* free=2, ask=4 => returns 2 */
    CHECK(n == 2, "second partial write returns 2");

    rb_destroy(rb);
}

void test_clear(void) {
    printf("--- clear ---\n");
    ringbuf_t *rb = rb_create(4);
    uint8_t a[] = {9,8,7};
    rb_write(rb, a, 3);
    rb_read(rb, (uint8_t[1]){0}, 1);   /* drain one */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear zeroes size");

    uint8_t out[4];
    size_t n = rb_read(rb, out, 4);
    CHECK(n == 0, "read on cleared ring returns 0");

    rb_destroy(rb);
}

void test_null_ops(void) {
    printf("--- NULL argument handling ---\n");
    uint8_t buf[] = {1};
    CHECK(rb_write(NULL, buf, 1) == 0, "rb_write(NULL)==0");
    CHECK(rb_read(NULL, buf, 1) == 0, "rb_read(NULL)==0");
    CHECK(rb_peek(NULL, buf, 1) == 0, "rb_peek(NULL)==0");
    CHECK(rb_size(NULL) == 0, "rb_size(NULL)==0");
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL)==0");
    rb_clear(NULL);                     /* no crash */
}

void test_empty_input(void) {
    printf("--- zero-length input ---\n");
    ringbuf_t *rb = rb_create(4);
    size_t n = rb_write(rb, NULL, 0);
    CHECK(n == 0, "write NULL/0 returns 0");

    (void)rb;
    n = rb_read(NULL, NULL, 0);
    CHECK(n == 0, "read NULL/0 returns 0");
    rb_destroy(rb);
}

void test_create_invalid(void) {
    printf("--- invalid create ---\n");
    CHECK(rb_create(0) == NULL, "create(0) returns NULL");
}

int main(void) {
    test_basic_create_destroy();
    test_write_read();
    test_wraparound_write();
    test_wraparound_peek();
    test_partial_write();
    test_clear();
    test_null_ops();
    test_empty_input();
    test_create_invalid();

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
