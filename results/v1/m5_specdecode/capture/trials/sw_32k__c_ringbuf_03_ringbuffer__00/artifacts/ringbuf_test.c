#include "ringbuf.h"
#include <stdio.h>
#include <string.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } else { printf("PASS: %s\n", msg); } \
} while(0)

int main(void)
{
    /* NULL operations should be safe */
    CHECK(rb_create(0) == NULL, "rb_create(0) returns NULL");
    rb_destroy(NULL); /* should not crash */
    CHECK(rb_size(NULL) == 0, "rb_size(NULL) == 0");
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL) == 0");

    /* Basic create/destroy */
    ringbuf_t *rb = rb_create(8);
    CHECK(rb != NULL, "rb_create(8) succeeds");
    CHECK(rb_capacity(rb) == 8, "capacity is 8");
    CHECK(rb_size(rb) == 0, "new buffer is empty");

    /* Write and read */
    uint8_t data[] = {1,2,3,4,5};
    size_t w = rb_write(rb, data, 5);
    CHECK(w == 5, "write 5 bytes returns 5");
    CHECK(rb_size(rb) == 5, "size is 5 after write");

    uint8_t rdbuf[8];
    size_t r = rb_read(rb, rdbuf, 5);
    CHECK(r == 5, "read 5 bytes returns 5");
    CHECK(memcmp(rdbuf, data, 5) == 0, "read data matches write data");
    CHECK(rb_size(rb) == 0, "size is 0 after read");

    /* Wrap-around: fill beyond capacity boundary */
    rb_clear(rb);
    uint8_t a[] = {10,11,12,13};
    w = rb_write(rb, a, 4);
    CHECK(w == 4, "write first chunk");
    CHECK(rb_size(rb) == 4, "size is 4");

    rb_read(rb, rdbuf, 2); /* consume first 2 */
    CHECK(rb_size(rb) == 2, "size is 2 after partial read");

    uint8_t b[] = {20,21,22,23};
    w = rb_write(rb, b, 4);
    CHECK(w == 4, "write wraps correctly");
    CHECK(rb_size(rb) == 6, "size is 6 after wrap write");

    /* Read back should get oldest first */
    r = rb_read(rb, rdbuf, 6);
    CHECK(r == 6, "read 6 wrapped bytes");
    CHECK(rdbuf[0]==12 && rdbuf[1]==13 && rdbuf[2]==20 &&
          rdbuf[3]==21 && rdbuf[4]==22 && rdbuf[5]==23, "wrapped read order correct");

    /* Peek */
    rb_clear(rb);
    rb_write(rb, (uint8_t[]){99}, 1);
    uint8_t p;
    size_t pr = rb_peek(rb, &p, 1);
    CHECK(pr == 1 && p == 99, "peek works");
    CHECK(rb_size(rb) == 1, "peek doesn't remove data");

    /* Fill entirely */
    rb_clear(rb);
    uint8_t bf[8];
    memset(bf, 0xAA, 8);
    w = rb_write(rb, bf, 8);
    CHECK(w == 8, "fill to capacity");
    w = rb_write(rb, (uint8_t[]){1}, 1);
    CHECK(w == 0, "write on full buffer returns 0");

    /* Write larger than free space */
    rb_clear(rb);
    uint8_t big[20];
    memset(big, 0xBB, 20);
    w = rb_write(rb, big, 20);
    CHECK(w == 8, "oversized write stores only what fits");

    /* Clear */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear empties buffer");

    rb_destroy(rb);
    rb_destroy(NULL); /* safe */

    printf("\nAll tests passed!\n");
    return 0;
}
