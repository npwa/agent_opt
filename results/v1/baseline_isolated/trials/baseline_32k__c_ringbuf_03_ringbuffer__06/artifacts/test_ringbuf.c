#include "ringbuf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    /* Test 1: create/destroy/capacity */
    ringbuf_t *rb = rb_create(16);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 16);
    assert(rb_size(rb) == 0);

    /* Test 2: write & read */
    uint8_t wdata[] = {1,2,3,4,5};
    size_t n = rb_write(rb, wdata, sizeof(wdata));
    assert(n == 5);
    assert(rb_size(rb) == 5);

    uint8_t rdata[5];
    n = rb_read(rb, rdata, sizeof(rdata));
    assert(n == 5);
    assert(memcmp(rdata, wdata, 5) == 0);
    assert(rb_size(rb) == 0);

    /* Test 3: wrap-around write */
    uint8_t a[] = {10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
    assert(rb_write(rb, a, 15) == 15);
    assert(rb_size(rb) == 15);

    /* Write one more to fill it */
    uint8_t x = 99;
    assert(rb_write(rb, &x, 1) == 1);
    assert(rb_size(rb) == 16);

    /* Cannot write more */
    assert(rb_write(rb, &x, 1) == 0);

    /* Read them all back in order */
    uint8_t buf[16];
    size_t count;
    count = rb_read(rb, buf, 5);
    assert(count == 5);
    assert(memcmp(buf, a, 5) == 0); /* oldest bytes first */

    /* Next 11 should be: remaining bytes from buf[5..14], then overflow byte at buf[15]=99 */
    uint8_t expected[] = {15,16,17,18,19,20,21,22,23,24,99};
    assert((int)sizeof(expected) == 11);
    uint8_t buf2[11];
    count = rb_read(rb, buf2, 11);
    assert(count == 11);
    for (int i = 0; i < 11; i++)
        assert(buf2[i] == expected[i]);

    /* Test 4: peek doesn't change size */
    assert(rb_size(rb) == 1); /* only 99 left */
    uint8_t pv = 0;
    rb_peek(rb, &pv, 1);
    assert(pv == 99);
    assert(rb_size(rb) == 1);

    /* Test 5: clear */
    rb_clear(rb);
    assert(rb_size(rb) == 0);

    /* Test 6: NULL safety */
    rb_destroy(NULL);
    assert(rb_write(NULL, wdata, 5) == 0);
    assert(rb_read(NULL, rdata, 5) == 0);
    assert(rb_peek(NULL, rdata, 5) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);
    rb_clear(NULL);

    /* Test 7: invalid capacity */
    rb_destroy(rb_create(0));

    printf("All tests passed.\n");
    return 0;
}
