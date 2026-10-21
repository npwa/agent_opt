#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int pass = 1;

    /* Test rb_create / rb_capacity */
    {
        ringbuf_t *rb = rb_create(16);
        if (!rb || rb_capacity(rb) != 16) { pass = 0; }
        else { printf("PASS: create/capacity\n"); }

        /* null creates must fail */
        if (rb_create(0) == NULL && rb_create((size_t)-1) == NULL) {
            printf("PASS: invalid capacity returns NULL\n");
        } else { pass = 0; }

        /* Test rb_clear / rb_destroy(NULL) */
        rb_destroy(NULL);          /* must not crash */
        rb_clear(rb);
        if (rb_size(rb) != 0 || rb_size(NULL) != 0) { pass = 0; }
        else { printf("PASS: clear/size(NULL)\n"); }
    }

    /* Basic write-read round trip */
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t out[8];
        uint8_t wdata[] = {1,2,3,4,5};
        size_t n;

        if (rb_size(rb) != 0) { pass = 0; }
        else { printf("PASS: new buffer is empty\n"); }

        n = rb_write(rb, wdata, 5);
        if (n != 5 || rb_size(rb) != 5) { pass = 0; }
        else { printf("PASS: write 5 bytes\n"); }

        n = rb_read(rb, out, 5);
        if (n != 5 || memcmp(out, wdata, 5) != 0) { pass = 0; }
        else { printf("PASS: read 5 bytes round-trip\n"); }

        if (rb_size(rb) != 0) { pass = 0; }
        else { printf("PASS: buffer empty after full drain\n"); }

        rb_destroy(rb);
    }

    /* Wrap-around test with capacity 4 */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t out[4], wdata[] = {10,20};
        size_t n;

        if (rb_size(NULL) != 0) { pass = 0; }
        else { printf("PASS: size(NULL)==0\n"); }

        rb_write(rb, wdata, 2);      /* [10,20,_,_] */
        rb_read(rb, out, 1);          /* read 10 => [_,20,_,_] */
        if (out[0] != 10) { pass = 0; }
        else { printf("PASS: peek/write/read mix\n"); }

        rb_write(rb, wdata, 2);      /* wrap-around: [_ ,10,20] + tail advances past capacity */
        n = rb_read(rb, out, 4);
        if (n != 3) { pass = 0; }
        else if (out[0] != 20 || out[1] != 10 || out[2] != 20) { pass = 0; }
        else { printf("PASS: wrap-around read %zu bytes\n", n); }

        /* Write larger than remaining space */
        rb_write(rb, wdata, 5);      /* only 1 byte fits (count was 3 of 4) */
        if (rb_size(rb) != rb_capacity(rb)) { pass = 0; }
        else { printf("PASS: partial write fills buffer\n"); }

        /* NULL write/read is a no-op */
        if (rb_write(NULL, wdata, 5) != 0 || rb_read(NULL, out, 4) != 0) { pass = 0; }
        else { printf("PASS: NULL args return 0\n"); }

        /* Peek test */
        {
            ringbuf_t *rb2 = rb_create(4);
            uint8_t w[]= {7,8}, r[4], p[4];
            rb_write(rb2, w, 2);
            if (rb_peek(rb2, p, 2) != 2 || memcmp(p, w, 2) != 0) { pass = 0; }
            else { printf("PASS: peek returns copy\n"); }

            rb_read(rb2, r, 1);          /* drain one */
            if (rb_peek(rb2, p, 4) != 1 || p[0] != w[1]) { pass = 0; }
            else { printf("PASS: peek after partial read\n"); }

            /* peek NULL */
            if (rb_peek(NULL, p, 4) != 0) { pass = 0; }
            else { printf("PASS: peek(NULL)==0\n"); }

            rb_destroy(rb2);
        }

        /* capacity returns 0 for NULL */
        if (rb_capacity(NULL) != 0) { pass = 0; }
        else { printf("PASS: capacity(NULL)==0\n"); }

        rb_destroy(rb);
    }

    printf("\n%s\n", pass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return pass ? 0 : 1;
}
