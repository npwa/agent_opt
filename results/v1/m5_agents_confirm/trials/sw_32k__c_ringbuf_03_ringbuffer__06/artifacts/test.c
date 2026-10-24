#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

int main(void)
{
    /* rb_create / rb_capacity / rb_size / rb_clear */
    ringbuf_t *rb = rb_create(8);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 8);
    assert(rb_size(rb) == 0);

    rb_destroy(NULL);
    assert(rb_create(0) == NULL);

    /* Basic write + read */
    uint8_t w[] = {1,2,3,4,5};
    size_t n;
    n = rb_write(rb, w, sizeof(w));
    assert(n == 5);
    assert(rb_size(rb) == 5);

    /* Clear and do wrap-around test cleanly */
    rb_clear(rb);

    /* Write 6 bytes: head=6, count=6, tail=0 */
    uint8_t b[] = {30,40,50,60,70,80};
    n = rb_write(rb, b, sizeof(b));
    assert(n == 6);

    /* Read 2: tail=2, count=4 */
    uint8_t tmp[10];
    size_t nr = rb_read(rb, tmp, 2);
    assert(nr == 2 && tmp[0] == 30 && tmp[1] == 40);

    /* Write more: we have head=6, space=2 (indices 6,7). 
       Write 4 bytes - first 2 go to [6][7], next 2 wrap to [0][1]. */
    uint8_t c[] = {90,91,92,93};
    n = rb_write(rb, c, sizeof(c));
    assert(n == 4);   /* space was only 2... wait, after read count=4, so space=8-4=4 */
    assert(n == 4 && "write all bytes fit");

    /* Now: tail=2 head=(6+4)%8=2 count=8 -- actually we need to trace carefully */
    /* After write of 4: buf[6]=90,buf[7]=91, then wrap: buf[0]=92,buf[1]=93. head becomes (6+4)%8=2. count was 4, now = 4+4=8? No, capacity is only 8 and we had count=4 + wrote 4 = 8 === capacity */
    /* tail=2, head=2, count=8 -- buffer is full with all data wrapping around! */

    /* Read all - tail must cross head */
    nr = rb_read(rb, tmp, 10);
    assert(nr == 8);
    /* Expected order: buf[2],buf[3],buf[4],buf[5] then buf[6],buf[7],buf[0],buf[1] */
    assert(tmp[0] == 50 && tmp[1] == 60 && tmp[2] == 70 && tmp[3] == 80);
    assert(tmp[4] == 90 && tmp[5] == 91 && tmp[6] == 92 && tmp[7] == 93);

    /* Peek without removing */
    rb_clear(rb);
    uint8_t e[] = {99};
    assert(rb_write(rb, e, 1) == 1);
    uint8_t p2;
    size_t np = rb_peek(rb, &p2, 1);
    assert(np == 1 && p2 == 99);
    assert(rb_size(rb) == 1);

    /* write larger than free space */
    uint8_t big[256];
    rb_clear(rb);
    n = rb_write(rb, big, sizeof(big));
    assert(n == 8);   /* only capacity bytes fit */

    printf("All tests passed\n");
    return 0;
}
