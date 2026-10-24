#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void) {
    int failed = 0;

    /* Create */

    ringbuf_t *rb = rb_create(8);
    if (!rb || rb_capacity(rb) != 8) { printf("FAIL: create\n"); return 1; }

    /* Clear and size */
    rb_clear(rb);
    if (rb_size(rb) != 0) { printf("FAIL: clear/size\n"); return 1; }

    /* Write 5 bytes, read them back */
    uint8_t w[5] = {1,2,3,4,5};
    size_t n = rb_write(rb, w, 5);
    if (n != 5) { printf("FAIL: write %zu\n", n); return 1; }
    if (rb_size(rb) != 5) { printf("FAIL: size after write\n"); return 1; }

    /* Peek */
    uint8_t peek[3];
    if (rb_peek(rb, peek, 3) != 3) { printf("FAIL: peek\n"); return 1; }
    if (peek[0] != 1 || peek[1] != 2 || peek[2] != 3) { printf("FAIL: peek vals\n"); return 1; }

    /* Read & check order */
    uint8_t rbuf[5];
    if (rb_read(rb, rbuf, 5) != 5) { printf("FAIL: read\n"); return 1; }
    for (int i = 0; i < 5; i++) if (rbuf[i] != (uint8_t)(i + 1)) { printf("FAIL: read vals\n"); return 1; }

    /* Underflow / NULL args */
    if (rb_read(NULL, rbuf, 5) != 0) { printf("FAIL: rb_read(NUll)\n"); return 1; }
    if (rb_write(rb, NULL, 5) != 0) { printf("FAIL: rb_write(NUll)\n"); return 1; }

    /* Write more than capacity, expect partial */
    uint8_t big[16] = {0};
    for (int i = 0; i < 16; i++) big[i] = (uint8_t)(i + 50);
    n = rb_write(rb, big, 16);
    if (n != 8) { printf("FAIL: write partial %zu\n", n); return 1; }

    /* Read back */
    uint8_t rbuf2[8];
    rb_read(rb, rbuf2, 8);
    for (int i = 0; i < 8; i++) if (rbuf2[i] != (uint8_t)(i + 50)) { printf("FAIL: read partial vals\n"); return 1; }

    /* Wrap-around test */
    rb_clear(rb);
    uint8_t a[6] = {10,11,12,13,14,15};
    rb_write(rb, a, 6);
    uint8_t out[3];
    rb_read(rb, out, 3); /* removes first 3 */
    /* remaining: 13,14,15 then add more to wrap */
    uint8_t b[5] = {20,21,22,23,24};
    rb_write(rb, b, 5); /* now have 7 bytes, head wraps */
    uint8_t check[7];
    rb_read(rb, check, 7);
    int ok = (check[0]==13 && check[1]==14 && check[2]==15 &&
              check[3]==20 && check[4]==21 && check[5]==22 && check[6]==23);
    if (!ok) { printf("FAIL: wrap-around\n"); return 1; }

    /* Destroy NULL */
    rb_destroy(NULL);

    /* Invalid capacity */
    if (rb_create(0) != NULL) { printf("FAIL: create(0)\n"); return 1; }

    rb_destroy(rb);
    printf("All tests passed.\n");
    return 0;
}
