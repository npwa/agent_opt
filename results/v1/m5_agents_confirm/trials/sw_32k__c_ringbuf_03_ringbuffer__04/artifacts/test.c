#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int assert2(int expr, const char *msg) {
    if (!expr) { printf("FAIL: %s\n", msg); return 1; }
    printf("OK: %s\n", msg);
    return 0;
}

int main(void) {
    int fails = 0;

    /* Test NULL checks */
    fails += assert2(rb_create(0) == NULL, "rb_create(0) returns NULL");
    rb_destroy(NULL); /* should not crash */
    rb_clear(NULL);   /* should not crash */

    ringbuf_t *rb = rb_create(8);
    fails += assert2(rb != NULL, "rb_create(8) succeeds");
    fails += assert2(rb_capacity(rb) == 8, "capacity is 8");
    fails += assert2(rb_size(rb) == 0, "new buffer is empty");

    /* Fill */
    uint8_t data[] = {1,2,3,4,5,6,7,8};
    size_t n = rb_write(rb, data, 8);
    fails += assert2(n == 8, "write 8 bytes");
    fails += assert2(rb_size(rb) == 8, "size is 8 after write");

    /* Full */
    fails += assert2(rb_write(rb, data, 1) == 0, "full buffer returns 0");

    /* Peek */
    uint8_t peek_buf[3] = {0};
    n = rb_peek(rb, peek_buf, 3);
    fails += assert2(n == 3, "peek returns 3");
    fails += assert2(peek_buf[0]==1 && peek_buf[1]==2 && peek_buf[2]==3, "peek returns oldest first");

    /* Read back */
    uint8_t out[8];
    n = rb_read(rb, out, 5);
    fails += assert2(n == 5, "read 5 bytes");
    for (int i = 0; i < 5; i++) fails += assert2(out[i] == (uint8_t)(i+1), "bytes match after read");
    fails += assert2(rb_size(rb) == 3, "size is 3 after reading 5");

    /* Wrap-around: buffer had 5 bytes + we wrote 4 but only 3 fit to reach capacity=8 */
    n = rb_write(rb, data, 4);
    fails += assert2(n == 4, "wrap-around write fills within remaining space");
    fails += assert2(rb_size(rb) == 7, "size is 7 after wrapping write");

    /* Read everything back via wrap-around */
    n = rb_read(rb, out, 100);
    fails += assert2(n == 7, "read remaining (wrapping across two segments)");

    /* Empty */
    fails += assert2(rb_size(rb) == 0, "size is 0 after reading all");
    fails += assert2(rb_read(rb, out, 1) == 0, "read from empty returns 0");

    /* Clear during use */
    rb_write(rb, data, 4);
    rb_clear(rb);
    fails += assert2(rb_size(rb) == 0, "clear empties buffer");
    fails += assert2(rb_read(rb, out, 1) == 0, "read from cleared is empty");

    rb_destroy(rb);
    puts(fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return fails;
}
