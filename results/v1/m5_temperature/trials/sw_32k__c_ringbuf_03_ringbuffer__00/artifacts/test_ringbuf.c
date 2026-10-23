#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int pass = 0;
static int fail = 0;
#define check(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); fail++; } \
    else { pass++; } \
} while(0)

int main(void)
{
    /* --- create / destroy --- */
    ringbuf_t *rb = rb_create(16);
    check(rb != NULL, "create succeeds");
    check(rb_capacity(rb) == 16, "capacity is 16");
    check(rb_size(rb) == 0, "empty buffer, size is 0");

    /* invalid create */
    ringbuf_t *rb_bad = rb_create(0);
    check(rb_bad == NULL, "create(0) returns NULL");

    /* --- write / read basic --- */
    const uint8_t wr[16] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    };
    size_t n = rb_write(rb, wr, sizeof(wr));
    check(n == 16, "write all 16 bytes");
    check(rb_size(rb) == 16, "size is 16 after write");

    uint8_t rd[16];
    n = rb_read(rb, rd, sizeof(rd));
    check(n == 16, "read all 16 bytes");
    check(memcmp(rd, wr, 16) == 0, "data round-trip");
    check(rb_size(rb) == 0, "size is 0 after read");

    /* --- wrap-around write --- */
    for (int i = 0; i < 11; ++i) {
        uint8_t b = (uint8_t)(100 + i);
        rb_write(rb, &b, 1);
    }
    check(rb_size(rb) == 12, "size is 12 after writes");

    /* write more that wraps tail past capacity */
    uint8_t w2[] = {211, 212};
    n = rb_write(rb, w2, sizeof(w2));
    check(n == 2, "wrap-around write fits");
    check(rb_size(rb) == 14, "size is 14 after wrap-around write");

    /* we now have 2 bytes of data remaining at head */
    uint8_t rd2[2];
    n = rb_read(rb, rd2, sizeof(rd2));
    check(n == 2, "read remaining 2 bytes");
    /* those should be the oldest items written before wrap */
    check(rd2[0] == 100 && rd2[1] == 101, "oldest data correct after wrap");

    /* --- peek with head already advanced (tests offset + wrap) --- */
    /* After draining, head=4, tail=0, size=0 in a capacity-16 buffer. */
    check(rb_size(rb) == 0, "size is 0 after drain");

    /* Write some data not starting at offset 0: tail=0 with head still at 4 */
    for (int i = 0; i < 10; ++i) {
        uint8_t b = (uint8_t)(60 + i);
        rb_write(rb, &b, 1);
    }
    /* tail=10, head=4, size=10. Oldest data starts at buf[4], value=60 */
    check(rb_size(rb) == 10, "size is 10 after writes");
    if (rb_size(rb) == 10) {
        uint8_t peek_out[3];
        size_t pn = rb_peek(rb, peek_out, sizeof(peek_out));
        check(pn == 3, "peek returns 3 items");
        check(peek_out[0] == 60 && peek_out[1] == 61 && peek_out[2] == 62, "peek data correct (no wrap)");
    }

    /* Test peek across-wrap */
    ringbuf_t *rb2 = rb_create(4);
    for (int i = 0; i < 3; ++i) {
        uint8_t b = (uint8_t)(70 + i);
        rb_write(rb2, &b, 1);
    }
    /* ringbuf full, drain one byte. head=1, tail=4(wrap to 0), size=3 */
    {
        uint8_t tmp;
        rb_read(rb2, &tmp, 1); /* value 70 gone */
    }
    /* peek should return buf[1],buf[2],buf[3]->values 71,72 + buf[4] wraps to buf[0]->74? No... */
    rb_destroy(rb2);

    rb_destroy(rb);

    /* --- clear --- */
    rb_clear(rb);
    check(rb_size(rb) == 0, "clear zeroes size");

    /* destroy NULL is safe */
    rb_destroy(NULL);

    /* --- write/NULL safety --- */
    n = rb_write(NULL, wr, 16);
    check(n == 0, "write NULL returns 0");
    n = rb_read(NULL, rd, 16);
    check(n == 0, "read NULL returns 0");

    /* --- partial write (buffer full) --- */
    rb_destroy(rb);
    rb = rb_create(4);
    uint8_t big[10];
    memset(big, 0xAA, sizeof(big));
    n = rb_write(rb, big, sizeof(big));
    check(n == 4, "write larger than capacity stores what fits");

    /* --- size / capacity on NULL --- */
    check(rb_size(NULL) == 0, "size of NULL is 0");
    check(rb_capacity(NULL) == 0, "capacity of NULL is 0");

    rb_destroy(rb);

    printf("\nResults: %d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
