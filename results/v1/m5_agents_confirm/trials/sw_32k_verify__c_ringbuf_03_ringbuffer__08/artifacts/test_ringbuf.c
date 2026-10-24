#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    /* Edge: NULL rb_destroy is OK */
    rb_destroy(NULL);

    /* Edge: invalid create */
    assert(rb_create(0) == NULL);

    ringbuf_t *rb = rb_create(4);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 4);
    assert(rb_size(rb) == 0);

    /* Fill the buffer */
    uint8_t inp[] = {1, 2, 3, 4};
    size_t n = rb_write(rb, inp, 4);
    assert(n == 4);
    assert(rb_size(rb) == 4);

    /* Buffer full: write more returns 0 */
    n = rb_write(rb, inp, 1);
    assert(n == 0);

    /* Peek without removing */
    uint8_t peeked[4];
    assert(rb_peek(rb, peeked, 4) == 4);
    assert(memcmp(peeked, inp, 4) == 0);
    assert(rb_size(rb) == 4);  /* still full */

    /* Read all back */
    uint8_t out[4];
    assert(rb_read(rb, out, 4) == 4);
    assert(memcmp(out, inp, 4) == 0);
    assert(rb_size(rb) == 0);

    /* Write after drain (tests wrap-around from head/tail == capacity) */
    uint8_t more[] = {7, 8};
    assert(rb_write(rb, more, 2) == 2);

    /* Read and write alternately causing many wraps */
    rb_clear(rb);
    for (int i = 0; i < 100; i++) {
        uint8_t b = (uint8_t)(i & 0xFF);
        size_t w = rb_write(rb, &b, 1);
        assert(w == 1);
        if (rb_size(rb) == rb_capacity(rb)) {
            uint8_t r;
            (void)rb_read(rb, &r, 1);
            /* The oldest byte is the one written at iteration i - capacity + 1 */
            int expected_iter = i - (int)rb_capacity(rb) + 1;
            assert(r == (uint8_t)(expected_iter & 0xFF));
        }
    }

    /* Edge: NULL arguments */
    assert(rb_write(NULL, inp, 4) == 0);
    assert(rb_read(NULL, out, 4) == 0);
    assert(rb_peek(NULL, peeked, 4) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);

    rb_destroy(rb);

    printf("ALL TESTS PASSED\n");
    return 0;
}
