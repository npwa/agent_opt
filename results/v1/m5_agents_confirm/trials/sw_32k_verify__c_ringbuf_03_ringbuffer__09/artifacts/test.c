#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void) {
    /* 1. Create / destroy basic */
    ringbuf_t *rb = rb_create(8);
    assert(rb != NULL);
    assert(rb_size(rb) == 0);
    assert(rb_capacity(rb) == 8);

    /* 2. Write and read single bytes */
    uint8_t w[] = {1, 2, 3};
    size_t wrote = rb_write(rb, w, sizeof(w));
    assert(wrote == 3);
    assert(rb_size(rb) == 3);

    uint8_t r[3] = {0};
    size_t rd = rb_read(rb, r, sizeof(r));
    assert(rd == 3);
    assert(memcmp(r, w, 3) == 0);
    assert(rb_size(rb) == 0);

    /* 3. Wrap-around: fill past the boundary */
    uint8_t w2[] = {10,11,12,13,14,15,16,17};       /* exactly capacity = 8 */
    rb_write(rb, w2, sizeof(w2));                      /* fill it */
    assert(rb_size(rb) == 8);

    uint8_t r2[4];
    rb_read(rb, r2, 4);                               /* read first half */
    assert(rb_size(rb) == 4);
    for (int i = 0; i < 4; ++i)
        assert(r2[i] == w2[i]);

    /* write more – should wrap */
    uint8_t w3[] = {20, 21, 22, 23};
    rb_write(rb, w3, sizeof(w3));
    uint8_t r3[4];
    rb_read(rb, r3, 4);                               /* should get the new tail wrapped data */
    assert(memcmp(r3, w3, 4) == 0);

    /* 4. Partial write (fill exactly to capacity) */
    ringbuf_t *rb4 = rb_create(4);
    uint8_t a[] = {1,2};
    rb_write(rb4, a, sizeof(a));
    assert(rb_size(rb4) == 2);
    size_t n = rb_write(rb4, a, sizeof(a));            /* should write exactly 2 more */
    assert(n == 2);
    assert(rb_size(rb4) == 4);                          /* now full */

    /* 5. Write when full returns 0 */
    n = rb_write(rb4, a, sizeof(a));
    assert(n == 0);

    /* 6. Peek */
    uint8_t peeked[2];
    size_t pn = rb_peek(rb4, peeked, 2);
    assert(pn == 2);
    assert(memcmp(peeked, a, 2) == 0);                 /* oldest bytes untouched */
    assert(rb_size(rb4) == 4);                         /* not consumed */

    /* 7. Clear */
    rb_clear(rb4);
    assert(rb_size(rb4) == 0);

    /* 8. NULL-safety */
    assert(rb_write(NULL, a, 1) == 0);
    assert(rb_read(NULL, r3, 1) == 0);
    assert(rb_peek(NULL, peeked, 1) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);
    rb_destroy(NULL);                                  /* must not crash */

    /* 9. Invalid create */
    assert(rb_create(0) == NULL);

    /* 10. Capacity-limited write stores as much as fits */
    ringbuf_t *rbsmall = rb_create(3);
    uint8_t big[] = {99,98,97,96,95};
    n = rb_write(rbsmall, big, sizeof(big));
    assert(n == 3);                                    /* only 3 bytes fit */
    uint8_t peek2[3];
    rb_peek(rbsmall, peek2, 3);
    assert(peek2[0] == 99 && peek2[1] == 98 && peek2[2] == 97);

    rb_destroy(rbsmall);
    rb_destroy(rb4);
    rb_destroy(rb);

    printf("All tests passed.\n");
    return 0;
}
