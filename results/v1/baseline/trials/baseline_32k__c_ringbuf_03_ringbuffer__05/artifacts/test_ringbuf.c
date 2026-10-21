#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

int main(void) {
    /* Create */
    ringbuf_t *rb = rb_create(8);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 8);
    assert(rb_size(rb) == 0);

    /* Write 5 bytes, then read them back */
    uint8_t msg[] = {1, 2, 3, 4, 5};
    size_t wr;
    wr = rb_write(rb, msg, sizeof(msg));
    assert(wr == 5);
    assert(rb_size(rb) == 5);

    uint8_t rd_buf[8] = {0};
    size_t rd;
    rd = rb_read(rb, rd_buf, sizeof(rd_buf));
    assert(rd == 5);
    assert(memcmp(rd_buf, msg, 5) == 0);
    assert(rb_size(rb) == 0);

    /* Fill 7 bytes one at a time */
    uint8_t bb = 0xBB;
    size_t i;
    for (i = 0; i < 7; i++) {
        rb_write(rb, &bb, 1);
    }
    assert(rb_size(rb) == 7);

    /* Write 3 bytes but only 1 fits */
    uint8_t more[] = {0xCC, 0xDD, 0xEE};
    wr = rb_write(rb, more, sizeof(more));
    assert(wr == 1);
    assert(rb_size(rb) == 8);

    /* Read all — verify wrap-around order */
    memset(rd_buf, 0, sizeof(rd_buf));
    rd = rb_read(rb, rd_buf, 8);
    assert(rd == 8);
    uint8_t expected[] = {0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xCC};
    assert(memcmp(rd_buf, expected, 8) == 0);

    /* Buffer now empty */
    assert(rb_size(rb) == 0);
    memset(rd_buf, 0, sizeof(rd_buf));
    wr = rb_write(rb, more, sizeof(more));
    assert(wr == 3);
    rd = rb_read(rb, rd_buf, 12);
    assert(rd == 3);
    assert(memcmp(rd_buf, more, 3) == 0);

    /* Clear */
    rb_clear(rb);
    assert(rb_size(rb) == 0);

    /* Write msg, peek, verify peek is non-destructive */
    wr = rb_write(rb, msg, sizeof(msg));
    assert(wr == 5);
    uint8_t pbuf[3];
    size_t pk;
    pk = rb_peek(rb, pbuf, 3);
    assert(pk == 3);
    assert(memcmp(pbuf, msg, 3) == 0);
    assert(rb_size(rb) == 5); /* peek did not remove */

    /* Read after peek — should still get all 5 */
    memset(rd_buf, 0, sizeof(rd_buf));
    rd = rb_read(rb, rd_buf, 10);
    assert(rd == 5);
    assert(memcmp(rd_buf, msg, 5) == 0);

    /* Clear and write past capacity */
    rb_clear(rb);
    uint8_t big[20];
    memset(big, 0xFF, sizeof(big));
    wr = rb_write(rb, big, 20);
    assert(wr == 8); /* only capacity=8 fits */

    /* Write NULL is safe */
    assert(rb_write(NULL, msg, 5) == 0);
    assert(rb_read(NULL, rd_buf, 5) == 0);
    assert(rb_peek(NULL, pbuf, 3) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);

    /* Clear NULL is safe */
    rb_clear(NULL);

    /* Destroy NULL is safe */
    rb_destroy(NULL);

    rb_destroy(rb);

    printf("All tests passed.\n");
    return 0;
}
