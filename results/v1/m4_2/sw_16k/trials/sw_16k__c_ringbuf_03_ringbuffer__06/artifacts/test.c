#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ringbuf.h"

int main(void)
{
    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FAIL: rb_create\n"); return 1; }

    /* capacity */
    if (rb_capacity(rb) != 8) { printf("FAIL: capacity\n"); return 1; }

    /* NULL handling */
    if (rb_size(NULL) != 0 || rb_read(NULL, NULL, 0) != 0) {
        printf("FAIL: NULL size/read\n"); return 1;
    }
    rb_destroy(NULL); /* must not crash */

    /* write and read */
    uint8_t wr[8] = {1,2,3,4,5,6,7,8};
    size_t n = rb_write(rb, wr, sizeof(wr));
    if (n != 8) { printf("FAIL: write %zu\n", n); return 1; }

    uint8_t rd[8];
    if (rb_read(rb, rd, 8) != 8) { printf("FAIL: read\n"); return 1; }
    for (int i = 0; i < 8; i++) {
        if (rd[i] != (uint8_t)(i + 1)) { printf("FAIL: data mismatch[%d]\n", i); return 1; }
    }

    /* empty */
    if (rb_size(rb) != 0) { printf("FAIL: size empty\n"); return 1; }
    if (rb_read(rb, rd, 1) != 0) { printf("FAIL: read empty\n"); return 1; }

    /* wrap-around test */
    rb_write(rb, wr+0, 4);     /* fill 4 -> head=4, tail=0 */
    rb_read(rb, rd, 3);         /* drain 3   -> tail=3, size=1 */
    uint8_t seq[7] = {9,10,11,12,13,14,15};
    n = rb_write(rb, seq, sizeof(seq));
    /* only 7 slots free (8-cap - 1-size), but capacity already filled by 4 */
    /* available = 8-1=7. seq has 7 bytes, so all 7 write */
    if (n != 7) { printf("FAIL: wrapped write %zu\n", n); return 1; }
    if (rb_size(rb) != rb_capacity(rb)) { printf("FAIL: full after wrap\n"); return 1; }

    uint8_t got[8];
    if (rb_read(rb, got, 8) != 8) { printf("FAIL: after-wrap read\n"); return 1; }
    /* written order: wr[3] then seq all -> tail was at pos of index 2 in ring */
    /* Expected logical content: wr[3]={4}, followed by 6 remaining from a second write */

    /* clear */
    rb_clear(rb);
    if (rb_size(rb) != 0 || rb_capacity(rb) != 8) { printf("FAIL: clear\n"); return 1; }

    /* partial write at capacity */
    uint8_t full[8] = {0};
    n = rb_write(rb, full, 8);
    if (n != 8) { printf("FAIL: full-cap write\n"); return 1; }
    uint8_t extra[] = {99, 88};
    n = rb_write(rb, extra, 2);
    if (n != 0) { printf("FAIL: overflow should be 0 got %zu\n", n); return 1; }

    /* peek */
    rb_clear(rb);
    rb_write(rb, wr, 4);
    uint8_t peekd[4];
    size_t pn = rb_peek(rb, peekd, 4);
    if (pn != 4) { printf("FAIL: peek count\n"); return 1; }
    for (int i = 0; i < 4; i++) {
        if (peekd[i] != (uint8_t)(i + 1)) { printf("FAIL: peek data[%d]\n", i); return 1; }
    }
    /* peek must not change state */
    if (rb_size(rb) != 4) { printf("FAIL: peek should not change size\n"); return 1; }

    rb_destroy(rb);

    /* invalid create */
    if (rb_create(0) != NULL || rb_create(32) == NULL) {
        printf("FAIL: invalid/valid create\n"); return 1;
    }

    printf("OK\n");
    return 0;
}
