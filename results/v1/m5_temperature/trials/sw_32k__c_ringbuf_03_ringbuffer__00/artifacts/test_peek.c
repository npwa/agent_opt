#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    /* Test with known buffer layout */
    ringbuf_t *rb = rb_create(32);

    /* Write 4 bytes, then peek at head position without draining */
    for (int i = 0; i < 4; ++i) {
        uint8_t b = (uint8_t)(10 + i);
        rb_write(rb, &b, 1);
    }
    /* Now: head=0, tail=4, size=4 */

    uint8_t peek_out[3];
    size_t n = rb_peek(rb, peek_out, 3);
    printf("peek count: %zu (expect 3)\n", n);
    printf("peek data: %d %d %d (expect 10 11 12)\n",
           peek_out[0], peek_out[1], peek_out[2]);
    printf("size after peek: %zu (expect 4)\n", rb_size(rb));

    /* Drain all */
    uint8_t rd[4];
    rb_read(rb, rd, 4);
    printf("read: %d %d %d %d\n", rd[0], rd[1], rd[2], rd[3]);

    /* Now head != tail. Write some more */
    for (int i = 0; i < 20; ++i) {
        uint8_t b = (uint8_t)(50 + i);
        rb_write(rb, &b, 1);
    }
    printf("size after 20 writes: %zu\n", rb_size(rb));

    /* Peek first 3 */
    n = rb_peek(rb, peek_out, 3);
    printf("\npeek2 count: %zu (expect 3)\n", n);
    printf("peek2 data: %d %d %d (expect 14 50 51)\n",
           peek_out[0], peek_out[1], peek_out[2]);

    rb_destroy(rb);
    return 0;
}
