#include "ringbuf.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    ringbuf_t *rb;
    uint8_t buf[64];
    size_t r, w;

    /* --- create/destroy/capacity --- */
    rb = rb_create(8);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 8);
    assert(rb_size(rb) == 0);
    rb_destroy(NULL); /* safe */

    /* --- write 4, fill all --- */
    for (unsigned i = 0; i < 8; i++) buf[i] = (uint8_t)(i + 1);
    w = rb_write(rb, buf, 4); /* writes only 4: {1,2,3,4} */
    assert(w == 4 && rb_size(rb) == 4);

    /* --- write more when full --- */
    r = rb_write(rb, buf + 4, 4);
    assert(r == 0 && rb_size(rb) == 4);

    /* --- peek (doesn't remove) --- */
    uint8_t p;
    rb_peek(rb, &p, 1);
    assert(p == 1 && rb_size(rb) == 4);
    rb_peek(NULL, &p, 1); /* safe: returns 0 */

    /* --- read 2 oldest --- */
    r = rb_read(rb, buf, 1);
    assert(r == 1 && buf[0] == 1 && rb_size(rb) == 3);
    r = rb_read(rb, buf + 1, 10);       /* reads remaining 3: {2,3,4} */
    assert(r == 3 && buf[1]==2 && buf[2]==3 && buf[3]==4 && rb_size(rb) == 0);

    /* --- read from empty is 0 --- */
    r = rb_read(rb, buf, 10);
    assert(r == 0);

    /* --- wrap-around test (capacity=4 ring) --- */
    rb = rb_create(4);       /* ring of size 4 */

    /* fill [A,B,C,D] where A=10..D=13, head wraps to pos[0], tail=0, stored=4 */
    w = rb_write(rb, (uint8_t[]){10,11,12,13}, 4);
    assert(w == 4 && rb_size(rb) == 4);

    /* drain [A,B] -> stored=2, tail=2, head=0(already was), avail=2 */
    r = rb_read(rb, buf, 2);
    assert(r == 2 && buf[0]==10 && buf[1]==11 && rb_size(rb) == 2);

    /* write [X] -> avail=2 so 1 fits. stores at pos[head]=pos[0], head->1 */
    w = rb_write(rb, (uint8_t[]){99}, 1);
    assert(w == 1 && rb_size(rb) == 3);           /* stored = 2+1 = 3 */

    /* read all: oldest first from tail=2 through wrap: {pos[2]=12, pos[3]=13, pos[0]=99} */
    r = rb_read(rb, buf, 64);                         /* min(read=64,min(stored,64)) = stored=3 */
    assert(r == 3 && buf[0] == 12/*oldest*/ && buf[1]==13&&buf[2]==99/*newest*/);
    assert(rb_size(rb) == 0);

    printf("All tests passed!\n");
    return 0;
}
