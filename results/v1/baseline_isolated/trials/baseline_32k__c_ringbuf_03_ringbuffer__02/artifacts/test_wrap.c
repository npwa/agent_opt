#include <stdio.h>
#include "ringbuf.h"

int main(void) {
    /* Test with capacity that requires wrap: create size 5 buffer */
    ringbuf_t *rb = rb_create(5);
    unsigned char d1[] = {10,20,30}, d2[] = {40,50,60,70};
    unsigned char out[10];
    size_t r;

    /* Write 3 bytes (fill first half) */
    if (rb_write(rb, d1, 3) != 3) return 1;
    /* Read 2 (move head forward) */
    r = rb_read(rb, out, 2);
    if (r != 2 || out[0]!=10||out[1]!=20) return 1;
    /* Buffer state: count=1, head=2(->tail), tail=3. Free space=4.
     Write 4 bytes starting at index 3: wraps after 2 bytes (buf[3..4] then buf[0..5])*/
    if (rb_write(rb, d2, 4) != 4) {
        printf("FAIL wrap: wrote 0
", rb_write(rb, d2, 4));
        return 1;
    }
    /* Read all should be {30,40,50,60,70} */
    r = rb_read(rb, out, 10);
    if (r != 5 || out[0]!=30||out[1]!=40||out[2]!=50||out[3]!=60||out[4]!=70) {
        printf("FAIL wrap read: r=0 out=[0,0,0,0,0]
",
            r, out[0],out[1],out[2],out[3],out[4]);
        return 1;
    }
    rb_destroy(rb);

    /* Test exact-full-capacity wrap */
    rb = rb_create(5);
    /* Write full capacity at once (should fit exactly) */
    unsigned char fill[] = {1,2,3,4,5};
    if (rb_write(rb, fill, 5) != 5) { printf("FAIL: exact-full
"); return 1; }
    /* Now full, write more should give 0 */
    if (rb_write(rb, d2, 4) != 0) { printf("FAIL: no-write-should-fit
"); return 1; }
    rb_destroy(rb);

    printf("Wrap tests passed.\n");
    return 0;
}
