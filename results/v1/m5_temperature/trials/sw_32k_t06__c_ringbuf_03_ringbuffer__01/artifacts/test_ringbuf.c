#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

int main(void) {
    ringbuf_t *rb = rb_create(8);
    assert(rb != NULL);
    printf("capacity=%zu size=%zu\n", rb_capacity(rb), rb_size(rb));

    uint8_t wdata[] = {1,2,3,4,5};
    size_t n = rb_write(rb, wdata, 5);
    printf("wrote %zu, size=%zu\n", n, rb_size(rb));
    assert(n == 5);
    assert(rb_size(rb) == 5);

    uint8_t peeked[5];
    n = rb_peek(rb, peeked, 5);
    printf("peeked %zu: ", n);
    for (size_t i = 0; i < n; i++) printf("%u ", (unsigned)peeked[i]);
    printf("\n");
    assert(n == 5);
    assert(memcmp(peeked, wdata, 5) == 0);
    assert(rb_size(rb) == 5);

    uint8_t rdata[5];
    n = rb_read(rb, rdata, 5);
    printf("read %zu: ", n);
    for (size_t i = 0; i < n; i++) printf("%u ", (unsigned)rdata[i]);
    printf("\n");
    assert(n == 5);
    assert(memcmp(rdata, wdata, 5) == 0);
    assert(rb_size(rb) == 0);

    uint8_t buf2[8];
    memset(buf2, 0xAA, 8);
    n = rb_write(rb, buf2, 8);
    printf("wrote %zu into full buffer (expected 0)\n", n);
    assert(rb_size(rb) == 8);

    n = rb_write(rb, wdata, 5);
    printf("wrote %zu when full (expected 0)\n", n);
    assert(n == 0);

    uint8_t tmp[4];
    n = rb_read(rb, tmp, 4);
    printf("read %zu: ", n);
    for (size_t i = 0; i < n; i++) printf("%u ", (unsigned)tmp[i]);
    printf("\n");
    assert(rb_size(rb) == 4);
    n = rb_write(rb, wdata, 5);
    printf("wrote %zu after reading 4 (expected 4)\n", n);
    assert(n == 4);

    rb_read(rb, tmp, 4);
    uint8_t out[4];
    n = rb_read(rb, out, 4);
    printf("read %zu: ", n);
    for (size_t i = 0; i < n; i++) printf("%u ", (unsigned)out[i]);
    printf("\n");
    assert(n == 4);
    for (size_t i = 0; i < 4; i++)
        assert(out[i] == wdata[i]);

    rb_clear(rb);
    assert(rb_size(rb) == 0);

    rb_destroy(NULL);
    assert(rb_write(NULL, wdata, 5) == 0);
    assert(rb_read(NULL, rdata, 5) == 0);
    assert(rb_peek(NULL, peeked, 5) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);
    rb_clear(NULL);

    assert(rb_create(0) == NULL);

    rb_destroy(rb);

    printf("All tests passed.\n");
    return 0;
}
