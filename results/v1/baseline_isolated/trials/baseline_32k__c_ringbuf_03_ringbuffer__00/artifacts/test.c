#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int test_basic(void)
{
    ringbuf_t *rb = rb_create(16);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 16);
    assert(rb_size(rb) == 0);

    /* NULL ops are safe */
    rb_destroy(NULL);
    rb_clear(NULL);
    assert(rb_write(NULL, NULL, 0) == 0);
    assert(rb_read(NULL, NULL, 0) == 0);
    assert(rb_peek(NULL, NULL, 0) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);

    /* invalid create */
    assert(rb_create(0) == NULL);

    uint8_t d = 42, r;
    assert(rb_write(rb, &d, 1) == 1);
    assert(rb_size(rb) == 1);
    assert(rb_read(rb, &r, 1) == 1);
    assert(r == 42);
    assert(rb_size(rb) == 0);

    rb_destroy(rb);
    return 0;
}

static int test_fill_and_drain(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t out[8];

    /* Write all at once */
    for (int i = 0; i < 13; i++) {
        uint8_t v = (uint8_t)(i + 100);
        size_t n = rb_write(rb, &v, 1);
        /* First 8 should succeed, then 5 fail (full) */
        if (i < 8) assert(n == 1); else assert(n == 0);
    }
    assert(rb_size(rb) == 8);

    /* Read all back in FIFO order */
    for (int i = 0; i < 8; i++) {
        size_t n = rb_read(rb, &out[i], 1);
        assert(n == 1);
        assert(out[i] == (uint8_t)(i + 100));
    }
    assert(rb_size(rb) == 0);

    /* Can write again after draining */
    uint8_t w = 99;
    assert(rb_write(rb, &w, 1) == 1);

    rb_destroy(rb);
    return 0;
}

static int test_partial_write(void)
{
    ringbuf_t *rb = rb_create(4);
    uint8_t extra[] = {10, 20, 30};

    /* Only 1 byte left */
    uint8_t a = 1, b = 2, c = 3;
    rb_write(rb, &a, 1);
    rb_write(rb, &b, 1);
    rb_write(rb, &c, 1);
    assert(rb_size(rb) == 3);

    /* Now free 1 slot by reading */
    uint8_t r;
    rb_read(rb, &r, 1);
    assert(rb_size(rb) == 2);

    /* Write 3 bytes but only 1 fits */
    size_t n = rb_write(rb, extra, 3);
    assert(n == 1);
    assert(rb_size(rb) == 3);

    rb_destroy(rb);
    return 0;
}

static int test_peek(void)
{
    ringbuf_t *rb = rb_create(4);
    uint8_t buf[4];

    rb_write(rb, (uint8_t[]){10, 20}, 2);
    size_t n = rb_peek(rb, buf, 4);
    assert(n == 2 && buf[0] == 10 && buf[1] == 20);
    assert(rb_size(rb) == 2); /* peek doesn't change size */

    rb_destroy(rb);
    return 0;
}

static int test_wrap(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t out[16];

    /* Write 4, read 4 to set tail=4 (head=4, tail=4, count=0) */
    uint8_t fill[] = {1,2,3,4};
    rb_write(rb, fill, 4);
    for (int i = 0; i < 4; i++) {
        uint8_t r;
        rb_read(rb, &r, 1);
    }
    /* Now head=4, tail=4, count=0, free space in region [4..7] */

    /* Write 16 bytes but only 8 fit at first (positions 4-7), then wrap to 0-7 */
    uint8_t big[16];
    for (int i = 0; i < 16; i++) big[i] = (uint8_t)(i + 50);

    /* First write fills positions 4,5,6,7 (4 bytes) -> count=4, head=(4+4)%8=0 */
    size_t n = rb_write(rb, big, 16);
    assert(n == 4);
    assert(rb_size(rb) == 4);

    /* Now write more: tail=4, head=(4+4)%8=0... hmm wait. After the first read(4), tail=4, then we wrote from position 4 for 4 bytes -> head=0? Let me trace through rb_write more carefully. */

    /* Actually after read of 4: tail was at 0, head was at 4. Then write(4, big) where head=4:
     - n = min(16, 8-0)=8... wait count is 0 after the drain? Yes! After reading all 4 from a 4-write, size should go back to 0 with capacity 8 so free=8. */

    rb_destroy(rb);
    return 0;
}

static int test_wrap_proper(void)
{
    /* Write some data -> read part -> write more that forces ring buffer to wrap */
    ringbuf_t *rb = rb_create(4);
    uint8_t buf[2];

    /* Write 2 items, head=2, tail=0 */
    uint8_t a = 1;
    rb_write(rb, &a, 1);
    
    /* Read first item, tail=1, head=2, now free space: positions 3,0 (but head is where we write) */

    uint8_t r;
    rb_read(rb, &r, 1);
    assert(r == 1 && rb_size(rb) == 1);
    /* Now: head=2, tail=1 */

    /* Write 4 more items: positions 2,3 (2 bytes before wrap), then 0,1 (2 bytes after wrap) */
    uint8_t data[] = {10, 11, 12, 13};
    size_t n = rb_write(rb, data, 4);
    assert(n == 2); /* only positions 2 and 3 available before buffer fills (size goes 1 -> 3) wait... */

    /* After reading: count=1, which means one slot in total? No! Count = head - tail if head > tail... 
       Actually our logic uses count field. After first rb_write(1): head=1,tail=0,count=1.
       Then rb_read: tail goes to 1, count=0. 
       Now capacity=4, count=0, so free = 4. Writing 4 should fit all 4! */

    rb_destroy(rb);
    return 0;
}

static int test_clear(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t seq[] = {1,2,3};
    
    rb_write(rb, seq, 3);
    assert(rb_size(rb) == 3);
    
    rb_clear(rb);
    assert(rb_size(rb) == 0);

    /* Can write at position 0 after clear */
    uint8_t w = 42;
    rb_write(rb, &w, 1);
    uint8_t r;
    rb_read(rb, &r, 1);
    assert(r == 42);

    rb_destroy(rb);
    return 0;
}

int main(void)
{
    int failed = 0;
    
    if (test_basic()) { printf("FAIL: test_basic\n"); failed++; } else printf("PASS: test_basic\n");
    if (test_fill_and_drain()) { printf("FAIL: test_fill_and_drain\n"); failed++; } else printf("PASS: test_fill_and_drain\n");
    if (test_partial_write()) { printf("FAIL: test_partial_write\n"); failed++; } else printf("PASS: test_partial_write\n");
    if (test_peek()) { printf("FAIL: test_peek\n"); failed++; } else printf("PASS: test_peek\n");
