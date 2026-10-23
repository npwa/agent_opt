#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

#define TEST(name) void name(void)
#define RUN(name) do { printf("  %-40s", #name); name(); printf("OK\n"); } while(0)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); abort(); } \
} while(0)

TEST(create_destroy) {
    ringbuf_t *rb = rb_create(64);
    ASSERT(rb != NULL, "create failed");
    ASSERT(rb_capacity(rb) == 64, "wrong capacity");
    ASSERT(rb_size(rb) == 0, "should be empty");
    rb_destroy(rb);

    /* NULL is accepted */
    rb_destroy(NULL);

    /* capacity==0 must return NULL */
    ASSERT(rb_create(0) == NULL, "zero capacity should fail");
}

TEST(write_read_roundtrip) {
    uint8_t data[12] = {0xDE, 0xAD, 0xBE, 0xEF, 'H', 'i', '\0', 7, 3, 5, 1, 9};
    ringbuf_t *rb = rb_create(8);
    ASSERT(rb != NULL, "create");

    size_t n = rb_write(rb, data, 12);
    ASSERT(n == 8, "write should have stored only 8 bytes");
    ASSERT(rb_size(rb) == 8, "size should be 8");

    uint8_t out[8];
    memset(out, 0, sizeof(out));
    n = rb_read(rb, out, 16);   /* ask for more than available */
    ASSERT(n == 8, "read all 8 bytes");
    ASSERT(memcmp(out, data, 8) == 0, "data match");
    ASSERT(rb_size(rb) == 0, "should be empty now");

    /* reading from empty buffer returns 0 */
    uint8_t tmp;
    ASSERT(rb_read(rb, &tmp, 1) == 0, "read empty");

    rb_destroy(rb);
}

TEST(write_all_then_read_all) {
    ringbuf_t *rb = rb_create(4);
    uint8_t w[3] = {1, 2, 3};
    ASSERT(rb_write(rb, w, 3) == 3, "write 3");
    uint8_t r[3];
    ASSERT(rb_read(rb, r, 3) == 3, "read 3");
    ASSERT(r[0]==1 && r[1]==2 && r[2]==3, "data match");
    rb_destroy(rb);
}

TEST(wrapping_write) {
    ringbuf_t *rb = rb_create(4);

    /* fill partially */
    uint8_t a[2] = {10, 20};
    rb_write(rb, a, 2);       /* size=2, tail=0, head=2 */

    /* read one byte -> size=1, tail=1, head=2 */
    uint8_t b;
    rb_read(rb, &b, 1);       /* b==10 */
    ASSERT(b == 10, "peeked first");

    /* write 4 bytes (3 should fit) -> wraps around */
    uint8_t c[4] = {30, 40, 50, 60};   /* only 3 stored: size becomes 4 again */
    size_t n = rb_write(rb, c, 4);
    ASSERT(n == 3, "only 3 should fit");

    uint8_t out[4];
    rb_read(rb, out, 4);
    ASSERT(out[0] == 20 && out[1] == 30 && out[2] == 40 && out[3] == 50, "wrapped write/read");

    rb_destroy(rb);
}

TEST(peek) {
    ringbuf_t *rb = rb_create(16);
    uint8_t w[6] = {9, 8, 7, 6, 5, 4};
    rb_write(rb, w, 6);

    uint8_t p[3];
    size_t n = rb_peek(rb, p, 3);
    ASSERT(n == 3 && p[0]==9 && p[1]==8 && p[2]==7, "peek first three");
    ASSERT(rb_size(rb) == 6, "peek shouldn't remove data");

    uint8_t r[6];
    rb_read(rb, r, 6);       /* drain */

    n = rb_peek(rb, p, 3);
    ASSERT(n == 0, "peek empty");

    rb_destroy(rb);
}

TEST(clear) {
    ringbuf_t *rb = rb_create(4);
    uint8_t w[4] = {1,2,3,4};
    rb_write(rb, w, 4);
    ASSERT(rb_size(rb) == 4, "full");
    rb_clear(rb);
    ASSERT(rb_size(rb) == 0, "cleared");

    /* after clear we can reuse completely */
    uint8_t x[4] = {10,20,30,40};
    ASSERT(rb_write(rb, x, 4) == 4, "write after clear");
    rb_destroy(rb);
}

TEST(null_args) {
    ASSERT(rb_write(NULL, (uint8_t[]){1}, 1) == 0, "NULL write");
    ASSERT(rb_read(NULL, (uint8_t[]){0}, 1) == 0, "NULL read");
    ASSERT(rb_peek(NULL, (uint8_t*){0}, 1) == 0, "NULL peek");
    ASSERT(rb_size(NULL) == 0, "NULL size");
    ASSERT(rb_capacity(NULL) == 0, "NULL capacity");
    rb_clear(NULL);  /* no crash */
}

int main(void) {
    printf("Running tests:\n");
    RUN(create_destroy);
    RUN(write_read_roundtrip);
    RUN(write_all_then_read_all);
    RUN(wrapping_write);
    RUN(peek);
    RUN(clear);
    RUN(null_args);
    printf("All tests passed.\n");
    return 0;
}
