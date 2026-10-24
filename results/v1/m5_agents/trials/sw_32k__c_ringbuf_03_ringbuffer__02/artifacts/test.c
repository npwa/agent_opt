#include "ringbuf.h"
#include <stdio.h>
#include <string.h>

static int count = 0;
static int failures = 0;

#define CHECK(cond, msg) do { \
    count++; \
    if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); failures++; } \
} while (0)

int main(void)
{
    /* NULL / edge cases */
    CHECK(rb_create(0) == NULL, "create zero capacity returns NULL");
    CHECK(rb_create(1) != NULL, "create capacity 1 ok");
    rb_destroy(NULL); /* should not crash */

    ringbuf_t *rb = rb_create(4);
    CHECK(rb_size(rb) == 0, "new buffer is empty");
    CHECK(rb_capacity(rb) == 4, "capacity is 4");
    CHECK(rb_read(rb, NULL, 0) == 0, "read zero-len returns 0");
    CHECK(rb_write(rb, NULL, 0) == 0, "write zero-len returns 0");

    /* basic write / read */
    uint8_t wr[] = {1, 2, 3, 4};
    size_t n = rb_write(rb, wr, sizeof(wr));
    CHECK(n == 4, "wrote 4 bytes");
    CHECK(rb_size(rb) == 4, "size is 4 after write");

    uint8_t rd[4];
    n = rb_read(rb, rd, sizeof(rd));
    CHECK(n == 4, "read 4 bytes");
    CHECK(memcmp(rd, wr, 4) == 0, "read values match written");
    CHECK(rb_size(rb) == 0, "size is 0 after read");

    /* wrap-around write */
    rb_clear(rb);
    uint8_t tmp[1];
    rb_read(rb, tmp, 2); /* partial read not possible, but drain what fits */
    rb_write(rb, (uint8_t[]){10}, 1);
    CHECK(rb_size(rb) == 1, "size=1 after single write");

    rb_read(rb, tmp, 1); /* consume it – head is now at capacity boundary */
    CHECK(rb_size(rb) == 0, "size=0 after consuming");

    /* write past the boundary (wrap-around test) */
    uint8_t w[] = {20, 30};
    n = rb_write(rb, w, sizeof(w));
    CHECK(n == 2, "write wrapped around successfully");
    CHECK(rb_size(rb) == 2, "size=2 after wrapping write");

    uint8_t r[2];
    rb_peek(rb, r, 2); /* peek without removing */
    CHECK(r[0] == 20 && r[1] == 30, "peek values match");
    CHECK(rb_size(rb) == 2, "peek did not consume");

    rb_read(rb, r, 2);
    CHECK(r[0] == 20 && r[1] == 30, "read after wrap matches");
    CHECK(rb_size(rb) == 0, "size=0 after read");

    /* write more than free space */
    rb_write(rb, (uint8_t[]){1, 2}, 2);
    n = rb_write(rb, (uint8_t[]){3, 4, 5}, 5);
    CHECK(n == 2, "write returns what fits (2)");
    CHECK(rb_size(rb) == 4, "size=4 after partial write");

    /* clear */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear makes empty");

    /* NULL rb on size / capacity */
    CHECK(rb_size(NULL) == 0, "rb_size(NULL) returns 0");
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL) returns 0");

    printf("%d tests, %d failures\n", count, failures);
    return failures > 0 ? 1 : 0;
}
