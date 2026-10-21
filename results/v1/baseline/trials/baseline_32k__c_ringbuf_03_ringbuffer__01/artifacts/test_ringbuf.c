#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

int main(void) {
    printf("=== Test rb_create/rb_destroy ===\n");
    ringbuf_t *rb = rb_create(0);
    assert(rb == NULL);

    ringbuf_t *r = rb_create(4);
    assert(r != NULL);
    assert(rb_size(r) == 0);
    assert(rb_capacity(r) == 4);
    rb_destroy(r);
    rb_destroy(NULL);

    printf("=== Test basic write/read ===\n");
    ringbuf_t *b = rb_create(4);
    
    uint8_t data_a[] = {10, 20};
    size_t n = rb_write(b, data_a, 2);
    assert(n == 2);
    assert(rb_size(b) == 2);

    uint8_t out[8] = {0};
    n = rb_read(b, out, 2);
    assert(n == 2 && out[0] == 10 && out[1] == 20);
    assert(rb_size(b) == 0);

    printf("=== Test full/buffer then empty ===\n");
    uint8_t data_b[] = {1, 2, 3, 4};
    n = rb_write(b, data_b, 4);
    assert(n == 4 && rb_size(b) == 4);
    assert(rb_write(b, data_a, 1) == 0);

    n = rb_read(b, out, 100);
    assert(n == 4 && out[0]==1 && out[1]==2 && out[2]==3 && out[3]==4);
    assert(rb_size(b) == 0 && rb_read(b, out, 1) == 0);

    printf("=== Test clear ===\n");
    uint8_t data_c[] = {5, 6};
    rb_write(b, data_c, 2);
    assert(rb_size(b) == 2);
    rb_clear(b);
    assert(rb_size(b) == 0);

    printf("=== Test peek ===\n");
    uint8_t pk[8] = {0};
    rb_write(b, data_c, 2);
    n = rb_peek(b, pk, 2);
    assert(n == 2 && pk[0]==5 && pk[1]==6);
    assert(rb_size(b) == 2);

    printf("=== Test wrap-around ===\n");
    rb_clear(b);
    uint8_t wr[] = {1, 2, 3};
    rb_write(b, wr, 3);
    assert(rb_size(b) == 3);

    memset(out, 0, sizeof(out));
    n = rb_read(b, out, 1);
    assert(n == 1 && out[0] == 1 && rb_size(b) == 2);

    uint8_t wr2[] = {4, 5, 6};
    n = rb_write(b, wr2, 3);
    assert(n == 2); /* avail = 4-2 = 2 */
    assert(rb_size(b) == 4);

    memset(out, 0, sizeof(out));
    n = rb_read(b, out, 10);
    assert(n == 4 && out[0]==2 && out[1]==3 && out[2]==4 && out[3]==5);

    printf("=== Test NULL safety ===\n");
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);
    assert(rb_write(NULL, data_a, 1) == 0);
    assert(rb_read(NULL, out, 1) == 0);
    assert(rb_peek(NULL, pk, 1) == 0);

    rb_destroy(b);
    
    printf("All tests passed!\n");
    return 0;
}
