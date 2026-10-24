#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ringbuf.h"

static int passed = 0;
static int failed = 0;

#define check(name, expr) do { \
    if (!(expr)) { printf("FAIL: %s\n", name); failed++; } \
    else { printf("PASS: %s\n", name); passed++; } \
} while (0)

int main(void)
{
    /* --- create / destroy / capacity --- */
    ringbuf_t *rb = rb_create(16);
    check("create non-zero returns non-NULL",  rb != NULL);
    check("capacity == 16",                    rb_capacity(rb) == 16);
    check("empty size == 0",                   rb_size(rb) == 0);

    ringbuf_t *rb_bad = rb_create(0);
    check("create zero returns NULL",          rb_bad == NULL);

    /* allow rb_destroy(NULL) */
    rb_destroy(NULL);   /* must not crash */
    rb_destroy(rb);     /* free it */

    /* --- write / read round-trip (no wrap) --- */
    rb = rb_create(8);
    uint8_t src[] = {0,1,2,3,4,5,6,7};
    size_t w = rb_write(rb, src, 8);
    check("write full capacity writes all",   w == 8);
    check("size after write == 8",            rb_size(rb) == 8);

    uint8_t dst[8] = {0};
    size_t r = rb_read(rb, dst, 8);
    check("read returns 8",                    r == 8);
    check("content matches",                   memcmp(src, dst, 8) == 0);
    check("size after read == 0",             rb_size(rb) == 0);

    /* --- reads/writes on NULL rb must return 0 --- */
    check("read NULL returns 0",              rb_read(NULL, dst, 8) == 0);
    check("write NULL returns 0",             rb_write(NULL, src, 8) == 0);

    /* --- partial write (data larger than free space) --- */
    w = rb_write(rb, src, 16);           /* 8 bytes fit in 8-cap buf */
    check("partial write writes what fits", w == 8);
    check("size after partial write == 8",  rb_size(rb) == 8);

    /* --- peek (doesn't remove) --- */
    r = rb_read(rb, dst, 8);             /* drain to clear */
    /* refill: write 3 bytes */
    uint8_t small[] = {10, 20, 30};
    w = rb_write(rb, small, 3);
    check("write 3",                       w == 3);

    uint8_t p[4] = {0};
    r = rb_peek(rb, p, 4);               /* peek beyond current count */
    check("peek returns 3",                r == 3);
    check("peek content correct",          memcmp(small, p, 3) == 0);
    check("size unchanged after peek",     rb_size(rb) == 3);  /* peek is non-destructive */

    /* --- clear --- */
    rb_clear(rb);
    check("size after clear == 0",        rb_size(rb) == 0);
    check("capacity still 8",              rb_capacity(rb) == 8);

    /* --- wrap-around test --- */
    /* fill and drain partially, then write more to force head/tail wrap */
    uint8_t filler[] = {99};
    w = rb_write(rb, filler, 1);         /* head=1, tail=0 */
    check("write 1 byte",                  w == 1);

    r = rb_read(rb, dst, 1);             /* drain everything, tail wraps to 1 */
    check("read 1",                        r == 1);
    check("size after read == 0",         rb_size(rb) == 0);
    check("tail wrapped back",             rb_size(rb) == 0);

    /* Now write 8 bytes -- they should all fit and wrap around head */
    w = rb_write(rb, src, 8);
    check("write after drain writes 8",   w == 8);

    uint8_t dst2[8] = {0};
    r = rb_read(rb, dst2, 8);
    check("round-trip after wrap ok",      r == 8 && memcmp(src, dst2, 8) == 0);

    /* --- NULL writes/reads return 0 --- */
    check("read with len=0 returns 0",     rb_read(rb, dst, 0) == 0);
    check("write with len=0 returns 0",    rb_write(rb, src, 0) == 0);

    /* --- full then write should yield 0 --- */
    for (int i = 0; i < 8; ++i)          /* fill to capacity (8 bytes in 8-cap buf) */
        rb_write(rb, filler, 1);
    check("size after filling == 8",      rb_size(rb) == 8);
    check("write after full returns 0",   rb_write(rb, src, 5) == 0);

    rb_destroy(rb);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
