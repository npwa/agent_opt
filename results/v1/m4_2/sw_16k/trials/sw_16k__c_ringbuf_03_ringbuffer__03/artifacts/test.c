#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    /* Test 1: create/destroy */
    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FAIL: rb_create\n"); return 1; }
    if (rb_capacity(rb) != 8) { printf("FAIL: capacity\n"); return 1; }
    if (rb_size(rb) != 0) { printf("FAIL: initial size\n"); return 1; }

    /* Test 2: write + read */
    uint8_t msg[] = {0,1,2,3,4,5,6,7};
    size_t w = rb_write(rb, msg, sizeof(msg));
    if (w != 8) { printf("FAIL: write %zu expected 8\n", w); return 1; }
    if (rb_size(rb) != 8) { printf("FAIL: size after write\n"); return 1; }

    uint8_t buf[8];
    size_t r = rb_read(rb, buf, sizeof(buf));
    if (r != 8) { printf("FAIL: read %zu\n", r); return 1; }
    if (rb_size(rb) != 0) { printf("FAIL: size after read\n"); return 1; }
    if (memcmp(buf, msg, 8) != 0) { printf("FAIL: data mismatch\n"); return 1; }

    /* Test 3: wrap-around write */
    uint8_t more[] = {10,11,12};
    w = rb_write(rb, more, 3);
    if (w != 3) { printf("FAIL: wrap-write %zu\n", w); return 1; }

    /* consume first 3 */
    uint8_t tmp[3];
    r = rb_read(rb, tmp, 3);
    if (r != 3 || tmp[0]!=0||tmp[1]!=1||tmp[2]!=2) { printf("FAIL: read-after-wrap\n"); return 1; }

    /* now write more to force wrap */
    uint8_t fill[] = {20,21,22,23,24};
    w = rb_write(rb, fill, 5);
    if (w != 5) { printf("FAIL: forcing-wrap-write %zu\n", w); return 1; }

    uint8_t out[8];
    r = rb_read(rb, out, 8);
    if (r != 8 || memcmp(out, more+3, 1) != 0) { printf("FAIL: partial consume test\n"); return 1; }
    /* remaining should be: 20,21,22,23,24 + one byte slot (garbage/zero but logically gone) */

    /* Test 4: partial write when full */
    ringbuf_t *rb2 = rb_create(3);
    uint8_t three[] = {1,2,3};
    w = rb_write(rb2, three, 3);
    if (w != 3) { printf("FAIL: fill\n"); return 1; }
    uint8_t extra[] = {99};
    w = rb_write(rb2, extra, 1);  /* full — should store 0 but spec says "what fits" => already full so 0 */
    if (w != 0) { printf("FAIL: partial-full returned %zu\n", w); return 1; }

    /* Test 5: peek */
    ringbuf_t *rb3 = rb_create(4);
    uint8_t d[] = {10,20};
    rb_write(rb3, d, 2);
    uint8_t peeked[4];
    size_t p = rb_peek(rb3, peeked, sizeof(peeked));
    if (p != 2 || peeked[0]!=10 || peeked[1]!=20) { printf("FAIL: peek\n"); return 1; }
    if (rb_size(rb3) != 2) { printf("FAIL: peek changed size\n"); return 1; }

    /* Test 6: clear */
    rb_clear(rb3);
    if (rb_size(rb3) != 0) { printf("FAIL: clear\n"); return 1; }

    /* Test 7: NULL safety */
    rb_destroy(NULL);
    if (rb_write(NULL, d, 2) != 0) { printf("FAIL: write(NULL)\n"); return 1; }
    if (rb_read(NULL, d, 2) != 0) { printf("FAIL: read(NULL)\n"); return 1; }

    rb_destroy(rb);
    rb_destroy(rb2);
    rb_destroy(rb3);

    /* Test 8: create with capacity=0 */
    if (rb_create(0) != NULL) { printf("FAIL: rb_create(0)\n"); return 1; }

    printf("ALL TESTS PASSED\n");
    return 0;
}
