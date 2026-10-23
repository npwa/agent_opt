#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    ringbuf_t *rb;
    uint8_t data[] = {1, 2, 3, 4, 5};
    uint8_t out[5];
    size_t n;
    int pass = 1;

    /* Test NULL handling */
    if (rb_create(0) != NULL) { printf("FAIL: rb_create(0)\n"); pass = 0; }
    if (rb_size(NULL) != 0) { printf("FAIL: rb_size(NULL)\n"); pass = 0; }
    if (rb_capacity(NULL) != 0) { printf("FAIL: rb_capacity(NULL)\n"); pass = 0; }
    rb_destroy(NULL); /* should not crash */

    /* Test create/destroy */
    rb = rb_create(8);
    if (!rb) { printf("FAIL: rb_create\n"); pass = 0; goto done; }
    if (rb_capacity(rb) != 8) { printf("FAIL: capacity\n"); pass = 0; }

    /* Test clear */
    rb_clear(rb);
    if (rb_size(rb) != 0) { printf("FAIL: clear size\n"); pass = 0; }

    /* Test write and read */
    n = rb_write(rb, data, 5);
    if (n != 5) { printf("FAIL: write %zu\n", n); pass = 0; }
    if (rb_size(rb) != 5) { printf("FAIL: size after write\n"); pass = 0; }

    n = rb_read(rb, out, 5);
    if (n != 5) { printf("FAIL: read %zu\n", n); pass = 0; }
    if (memcmp(out, data, 5) != 0) { printf("FAIL: read content\n"); pass = 0; }

    /* Test wrap-around */
    rb_write(rb, (uint8_t[]){10, 11}, 2);
    rb_write(rb, (uint8_t[]){12, 13, 14, 15, 16, 17}, 6);
    n = rb_read(rb, out, 8);
    if (n != 8) { printf("FAIL: wrap read %zu\n", n); pass = 0; }

    /* Test peek */
    rb_clear(rb);
    rb_write(rb, data, 5);
    n = rb_peek(rb, out, 3);
    if (n != 3) { printf("FAIL: peek %zu\n", n); pass = 0; }
    if (memcmp(out, data, 3) != 0) { printf("FAIL: peek content\n"); pass = 0; }
    if (rb_size(rb) != 5) { printf("FAIL: peek size unchanged\n"); pass = 0; }

    /* Test fill and partial write */
    rb_clear(rb);
    n = rb_write(rb, (uint8_t[]){1,2,3,4,5,6,7,8}, 8);
    if (n != 8) { printf("FAIL: fill %zu\n", n); pass = 0; }
    n = rb_write(rb, (uint8_t[]){9,10}, 2);
    if (n != 0) { printf("FAIL: full write %zu\n", n); pass = 0; }

done:
    rb_destroy(rb);
    printf("%s\n", pass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return pass ? 0 : 1;
}
