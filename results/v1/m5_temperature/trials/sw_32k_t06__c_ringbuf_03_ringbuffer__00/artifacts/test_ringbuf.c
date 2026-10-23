#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int fail = 0;

    if (rb_create(0) != NULL) fail = 1;
    if (rb_size(NULL) != 0) fail = 1;
    rb_destroy(NULL);

    ringbuf_t *rb = rb_create(4);
    if (!rb || rb_capacity(rb) != 4 || rb_size(rb) != 0) fail = 1;

    uint8_t wdata[] = {1, 2, 3, 4};
    if (rb_write(rb, wdata, 5) != 4) fail = 1;
    if (rb_size(rb) != 4) fail = 1;

    uint8_t peek[4];
    size_t pn = rb_peek(rb, peek, 4);
    if (pn != 4 || peek[0] != 1 || peek[3] != 4) fail = 1;

    uint8_t rdata[4];
    size_t rn = rb_read(rb, rdata, 4);
    if (rn != 4 || rdata[0] != 1 || rdata[3] != 4) fail = 1;
    if (rb_size(rb) != 0) fail = 1;

    /* Wrap-around: write 2, read 1, write 3 more */
    rb_write(rb, wdata, 2);
    uint8_t tmp[1];
    rb_read(rb, tmp, 1);
    uint8_t more[] = {30, 40, 50};
    size_t wn = rb_write(rb, more, 3);
    if (wn != 3) fail = 1;

    /* Buffer now full: [50,2,30,40] tail at pos 1 */
    uint8_t rbuf[4];
    rn = rb_read(rb, rbuf, 4);
    if (rn != 4 || rbuf[0] != 2 || rbuf[1] != 30 || rbuf[2] != 40 || rbuf[3] != 50) fail = 1;

    /* Write after read: head should be at pos 1 */
    uint8_t w2[] = {99};
    wn = rb_write(rb, w2, 1);
    if (wn != 1 || rbuf[0] != 2) fail = 1;

    /* Clear and re-use */
    rb_write(rb, wdata, 4);
    rb_clear(rb);
    if (rb_size(rb) != 0) fail = 1;

    /* Edge cases */
    if (rb_write(NULL, wdata, 4) != 0) fail = 1;
    if (rb_read(NULL, rdata, 4) != 0) fail = 1;
    if (rb_peek(NULL, peek, 4) != 0) fail = 1;
    if (rb_write(rb, NULL, 4) != 0) fail = 1;
    if (rb_read(rb, NULL, 4) != 0) fail = 1;
    if (rb_peek(rb, NULL, 4) != 0) fail = 1;

    rb_destroy(rb);

    printf(fail ? "FAILURES\n" : "ALL PASSED\n");
    return fail ? 1 : 0;
}
