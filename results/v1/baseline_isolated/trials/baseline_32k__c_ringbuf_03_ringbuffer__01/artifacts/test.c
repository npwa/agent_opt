#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ringbuf.h"

int main(void)
{
    int pass_count = 0;
    int fail_count = 0;

#define CHECK(expr, msg)                   \
    do {                                    \
        if (!(expr)) {                      \
            printf("FAIL: %s\n", msg);     \
            fail_count++;                   \
        } else {                            \
            pass_count++;                   \
        }                                   \
    } while (0)

    /* rb_create */
    ringbuf_t *rb = rb_create(16);
    CHECK(rb != NULL, "create");
    CHECK(rb_capacity(rb) == 16, "capacity after create");
    CHECK(rb_size(rb) == 0, "size after create");

    /* NULL / zero create */
    CHECK(rb_create(0) == NULL, "zero capacity rejected");

    rb_destroy(NULL);
    pass_count++;

    /* write */
    uint8_t in[] = {0, 1, 2, 3, 4};
    size_t w = rb_write(rb, in, sizeof(in));
    CHECK(w == 5, "write 5 bytes");
    CHECK(rb_size(rb) == 5, "size after write 5");

    /* NULL write */
    CHECK(rb_write(NULL, in, 1) == 0, "NULL write returns 0");
    CHECK(rb_write(rb, NULL, 1) == 0, "NULL data write returns 0");

    /* read */
    uint8_t out[5];
    size_t r = rb_read(rb, out, sizeof(out));
    CHECK(r == 5, "read 5 bytes");
    CHECK(memcmp(out, in, 5) == 0, "data match after read");
    CHECK(rb_size(rb) == 0, "size after drain");

    /* peek */
    w = rb_write(rb, in, sizeof(in));
    CHECK(w == 5, "write peek data");

    uint8_t peek_out[3];
    r = rb_peek(rb, peek_out, 3);
    CHECK(r == 3, "peek 3 bytes");
    CHECK(memcmp(peek_out, in, 3) == 0, "peek data matches");
    CHECK(rb_size(rb) == 5, "size unchanged after peek");

    /* clear */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "size after clear");

    /* Wrap-around test */
    ringbuf_t *sm = rb_create(4);
    CHECK(sm != NULL, "small create");

    for (int round = 0; round < 10; round++) {
        uint8_t data[] = {10, 20, 30};
        w = rb_write(sm, data, sizeof(data));
        CHECK(w == 3, "write fits in small buffer");

        uint8_t rdata[4];
        r = rb_read(sm, rdata, sizeof(rdata));
        CHECK(r == 3, "read from small buffer");
        CHECK(memcmp(rdata, data, 3) == 0, "small data match");
    }

    /* full buffer reject */
    uint8_t fill2[] = {5, 6};
    rb_write(sm, (uint8_t[]){7}, 1);
    w = rb_write(sm, fill2, sizeof(fill2));
    CHECK(w == 1, "write one more to fill");

    w = rb_write(sm, in, sizeof(in));
    CHECK(w == 0, "write to full returns 0");

    /* partial write cap */
    uint8_t big[] = {100, 101, 102, 103, 104};
    rb_clear(sm);
    w = rb_write(sm, big, sizeof(big));
    CHECK(w == 4, "write larger than free space caps");

    uint8_t check[4];
    r = rb_read(sm, check, 4);
    CHECK(r == 4, "read capped write");
    CHECK(memcmp(check, big, 4) == 0, "partial write data match");

    rb_destroy(sm);

    /* NULL queries */
    CHECK(rb_size(NULL) == 0, "rb_size(NULL)");
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL)");
    CHECK(rb_peek(NULL, out, 1) == 0, "rb_peek(NULL)");

    /* partial read leaves remaining */
    ringbuf_t *half = rb_create(8);
    uint8_t src[] = {10, 20, 30, 40};
    rb_write(half, src, sizeof(src));
    r = rb_read(half, check, 2);
    CHECK(r == 2 && rb_size(half) == 2, "partial read");

    uint8_t remaining[4];
    r = rb_read(half, remaining, 4);
    CHECK(r == 2 && memcmp(remaining, src + 2, 2) == 0, "residual read");

    rb_destroy(half);

    printf("done: %d passed, %d failed\n", pass_count, fail_count);
#undef CHECK
    return fail_count != 0 ? 1 : 0;
}
