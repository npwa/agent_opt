#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s at line %d\n", msg, __LINE__); return 1; } \
} while(0)

int main(void)
{
    /* Test rb_create / rb_destroy */
    ringbuf_t *rb = rb_create(8);
    CHECK(rb != NULL, "create");
    CHECK(rb_capacity(rb) == 8, "capacity");
    CHECK(rb_size(rb) == 0, "size empty");

    /* Test NULL handling */
    rb_destroy(NULL);       /* should not crash */
    CHECK(rb_size(NULL) == 0, "size NULL");
    CHECK(rb_capacity(NULL) == 0, "capacity NULL");
    CHECK(rb_write(NULL, NULL, 1) == 0, "write NULL");
    CHECK(rb_read(NULL, NULL, 1) == 0, "read NULL");
    CHECK(rb_peek(NULL, NULL, 1) == 0, "peek NULL");

    /* Test basic write + read */
    uint8_t data[] = {1, 2, 3, 4, 5};
    size_t w = rb_write(rb, data, sizeof(data));
    CHECK(w == 5, "write count");
    CHECK(rb_size(rb) == 5, "size after write");

    uint8_t out[5];
    size_t r = rb_read(rb, out, sizeof(out));
    CHECK(r == 5, "read count");
    CHECK(memcmp(out, data, 5) == 0, "read data");
    CHECK(rb_size(rb) == 0, "size after read");

    /* Test wrap-around write */
    uint8_t d1[] = {10, 11, 12, 13, 14, 15, 16, 17}; /* fills buffer */
    CHECK(rb_write(rb, d1, 8) == 8, "fill");
    uint8_t peek_out[3];
    size_t p = rb_peek(rb, peek_out, 3);
    CHECK(p == 3, "peek count");
    CHECK(memcmp(peek_out, d1, 3) == 0, "peek data");
    CHECK(rb_size(rb) == 8, "size after peek unchanged");

    /* Read 5, write more to trigger wrap */
    rb_read(rb, out, 5);
    uint8_t d2[] = {99};
    w = rb_write(rb, d2, 1);
    CHECK(w == 1, "write after partial read");

    /* Read all and verify order */
    uint8_t buf[8];
    r = rb_read(rb, buf, 8);
    CHECK(r == 4, "read remaining");
    CHECK(buf[0] == 15 && buf[1] == 16 && buf[2] == 17 && buf[3] == 99, "wrap order");

    /* Test write larger than free space */
    rb_clear(rb);
    w = rb_write(rb, data, 10); /* only 8 fit */
    CHECK(w == 8, "write partial");

    /* Test clear */
    rb_write(rb, data, 5);
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear");

    /* Test invalid create */
    CHECK(rb_create(0) == NULL, "create zero");

    /* Test wrap-around read */
    rb_destroy(rb);
    rb = rb_create(8);
    uint8_t a[] = {1, 2, 3, 4};
    rb_write(rb, a, 4);
    uint8_t out2[4];
    rb_read(rb, out2, 4); /* read [1,2,3,4] */
    uint8_t b[] = {5, 6, 7, 8};
    rb_write(rb, b, 4);  /* write at head=0 -> buffer has [5,6,7,8] */
    uint8_t result[4];
    r = rb_read(rb, result, 4);
    CHECK(r == 4, "read after wrap");
    CHECK(result[0] == 5 && result[1] == 6 && result[2] == 7 && result[3] == 8, "wrap read data");

    /* Test partial write then wrap-around read */
    rb_clear(rb);
    uint8_t c[] = {10, 11, 12, 13, 14, 15};
    w = rb_write(rb, c, 6);
    CHECK(w == 6, "write 6");
    uint8_t d[3];
    r = rb_read(rb, d, 3);
    CHECK(r == 3, "read 3");
    CHECK(d[0] == 10 && d[1] == 11 && d[2] == 12, "partial read");
    uint8_t e[] = {99, 100};
    w = rb_write(rb, e, 2); /* writes at tail=3: [14,15,99,100] */
    CHECK(w == 2, "write after partial");
    uint8_t result2[4];
    r = rb_read(rb, result2, 4);
    CHECK(r == 4, "read remaining");
    CHECK(result2[0] == 13 && result2[1] == 14 && result2[2] == 15 && result2[3] == 99, "wrap read order");

    rb_destroy(rb);

    printf("All tests passed.\n");
    return 0;
}
