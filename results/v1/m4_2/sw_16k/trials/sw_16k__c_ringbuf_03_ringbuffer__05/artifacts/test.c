#include <stdio.h>
#include <assert.h>
#include "ringbuf.h"

#define ASSERT_EQ(a, b) do { if ((a)!=(b)) { fprintf(stderr,"fail at line %d: %zu != %zu\n",__LINE__,(a),(b)); return 123; } } while(0)

int main(void) {
    /* Create / destroy */
    assert(rb_create(0) == NULL);
    ringbuf_t *rb = rb_create(4);
    ASSERT_EQ(rb_capacity(rb), 4);
    ASSERT_EQ(rb_size(rb), 0);

    /* Write and read full buffer */
    uint8_t w[] = {10,20,30};
    size_t n = rb_write(rb, w, 3);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(rb_size(rb), 3);
    uint8_t r[3];
    n = rb_read(rb, r, 3);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(r[0],10); ASSERT_EQ(r[1],20); ASSERT_EQ(r[2],30);

    /* Wrap-around: write half, read part, write more (overwrites tail region), drain */
    rb_write(rb, w+1, 2);         /* [20,30,_,_], head=2,tail=0 */
    r[0] = 0; r[1] = 0;
    rb_read(rb, r, 2);            /* read [20,30] -> tail becomes 2, nbytes=0 */

    /* Now write 4 bytes (full capacity) - wraps head around to before tail */
    uint8_t x[] = {11,22,33,44};
    n = rb_write(rb, x, 4);       /* buf:[11,_,33,44], head=(2+4)%4=0,tail=0 */
    ASSERT_EQ(n, 4);

    uint8_t y[4];
    n = rb_read(rb, y, 4);
    ASSERT_EQ(n,4);
    ASSERT_EQ(y[0],11); ASSERT_EQ(y[1],22); ASSERT_EQ(y[2],33); ASSERT_EQ(y[3],44);

    /* Peek doesn't change buffer */
    rb_clear(rb);
    rb_write(rb, w, 3);           /* [10,20,30] */
    uint8_t pz[6]; memset(pz,0xAA,sizeof(pz));
    ASSERT_EQ(rb_peek(rb, pz, 6), 3);
    ASSERT_EQ(pz[0],10); ASSERT_EQ(pz[1],20); ASSERT_EQ(pz[2],30);
    ASSERT_EQ(rb_size(rb), 3);    /* peek didn't change size */

    /* Clear */
    rb_clear(rb); ASSERT_EQ(rb_size(rb),0);
    rb_destroy(rb); rb_destroy(NULL);

    printf("all tests passed\n");
    return 0;
}
