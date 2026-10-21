#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int pass = 1;

    /* Test NULL/NULL ops don't crash */
    rb_destroy(NULL);
    if (rb_size(NULL) != 0 || rb_capacity(NULL) != 0) { pass = 0; }

    /* Create with invalid capacity */
    if (rb_create(0) != NULL) { pass = 0; }

    /* Basic create/destroy */
    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FAIL: create\n"); return 1; }
    if (rb_capacity(rb) != 8) { pass = 0; }
    if (rb_size(rb) != 0) { pass = 0; }

    /* Write all bytes */
    unsigned char data[] = {1,2,3,4,5,6,7,8};
    size_t w = rb_write(rb, data, 16);
    if (w != 8) { printf("FAIL: write full capacity wrote=%zu\n", w); pass = 0; }

    /* Write into full buffer */
    data[0] = 99;
    w = rb_write(rb, data, 1);
    if (w != 0) { pass = 0; }

    /* Read should give all 8 bytes */
    unsigned char out[8];
    size_t r = rb_read(rb, out, 8);
    if (r != 8) { printf("FAIL: read count=%zu\n", r); pass = 0; }
    for (int i = 0; i < 8; i++) {
        if (out[i] != (unsigned char)(i + 1)) {
            printf("FAIL: out[%d]=%d expected=%d\n", i, out[i], i + 1);
            pass = 0;
        }
    }

    /* Buffer should be empty */
    if (rb_size(rb) != 0) { pass = 0; }

    /* Ring: write after clear */
    rb_clear(rb);
    unsigned char data2[] = {10, 20, 30};
    w = rb_write(rb, data2, 3);
    if (w != 3 || rb_size(rb) != 3) { pass = 0; }

    /* Peek */
    unsigned char peek[4];
    r = rb_peek(rb, peek, 4);
    if (r != 3 || peek[0] != 10 || peek[1] != 20 || peek[2] != 30) { pass = 0; }

    /* Partial read then write more */
    rb_clear(rb);
    w = rb_write(rb, data2, 3);
    r = rb_read(rb, out, 2);
    if (r != 2 || out[0] != 10 || out[1] != 20) { pass = 0; }

    /* Wrap-around write: fill half, read some, then wrap */
    rb_clear(rb);
    w = rb_write(rb, data, 4);   /* puts {1..4} at buf[0..3], tail=4 */
    r = rb_read(rb, out, 2);     /* removes {1,2}, head=2, count=2, free_space=6 */
    r = rb_read(rb, out, 2);     /* removes {3,4}, head=4, count=0, free_space=8 */
    /* head=tail=4. Now write 5 bytes: tail is at 4, buf[4..7] has 4 slots, then wraps to buf[0] with 1 slot = 5 total */
    unsigned char data3[] = {50,60,70,80,90};
    w = rb_write(rb, data3, 5);
    if (w != 5) { printf("FAIL: wrap write wrote=%zu\n", w); pass = 0; }
    /* read should give [50,60,70,80,90] */
    r = rb_read(rb, out, 8);
    if (r != 5 || out[0]!=50||out[1]!=60||out[2]!=70||out[3]!=80||out[4]!=90) { pass = 0; }

    /* NULL write/read no crash */
    rb_write(NULL, data, 1);
    rb_read(NULL, out, 1);
    rb_peek(NULL, out, 1);

    rb_destroy(rb);

    if (pass) {
        printf("All tests passed.\n");
        return 0;
    } else {
        printf("FAILURES detected.\n");
        return 1;
    }
}
