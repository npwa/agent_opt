#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void) {
    /* Test 1: basic write/read round-trip */
    ringbuf_t *rb = rb_create(4);
    if (!rb || rb_capacity(rb) != 4) { puts("FAIL: create"); return 1; }

    uint8_t w[] = {0xA, 0x5B, 0xC2, 0x3D};
    size_t n = rb_write(rb, w, sizeof(w));
    if (n != 4) { printf("FAIL: write %zu\n", n); return 1; }

    uint8_t r[4];
    n = rb_read(rb, r, sizeof(r));
    if (n != 4 || memcmp(r, w, 4) != 0) { puts("FAIL: read"); return 1; }
    puts("PASS: basic write/read");

    /* Test 2: wrap-around write */
    rb_write(rb, &w[0], 3);
    uint8_t v[] = {0xFF};
    rb_write(rb, v, 1);
    n = rb_read(rb, r, sizeof(r));
    if (n != 4 || (r[0] != w[0] || r[1] != w[1] || r[2] != w[2] || r[3] != 0xFF)) {
        puts("FAIL: wrap-around"); return 1;
    }
    puts("PASS: wrap-around write/read");

    /* Test 3: partial write (not enough space) */
    rb_clear(rb);
    uint8_t w2[] = {1,2,3,4,5,6};
    n = rb_write(rb, w2, sizeof(w2));
    if (n != 4) { puts("FAIL: partial write"); return 1; }
    puts("PASS: partial write");

    /* Test 4: read returns max available */
    uint8_t r2[10];
    n = rb_read(rb, r2, sizeof(r2));
    if (n != 4) { puts("FAIL: full read"); return 1; }
    if (r2[0] != 1 || r2[1] != 2 || r2[2] != 3 || r2[3] != 4) { puts("FAIL: partial val"); return 1; }
    puts("PASS: read returns max available");

    /* Test 5: null safety */
    if (rb_size(NULL) != 0 || rb_capacity(NULL) != 0) { puts("FAIL: null size"); return 1; }
    rb_destroy(NULL);     /* must not crash */
    puts("PASS: null safety");

    /* Test 6: peek */
    uint8_t wd[] = {0x12, 0x34, 0x56};
    rb_write(rb, wd, sizeof(wd));
    uint8_t pk[3];
    n = rb_peek(rb, pk, 3);
    if (n != 3 || memcmp(pk, wd, 3) != 0) { puts("FAIL: peek"); return 1; }
    /* peek shouldn't change size */
    if (rb_size(rb) != 3) { puts("FAIL: peek mutates"); return 1; }
    n = rb_read(rb, pk, 3);
    if (n != 3 || memcmp(pk, wd, 3) != 0) { puts("FAIL: read after peek"); return 1; }
    puts("PASS: peek");

    /* Test 7: clear */
    rb_write(rb, &wd[0], sizeof(wd));
    rb_clear(rb);
    if (rb_size(rb) != 0) { puts("FAIL: clear"); return 1; }
    puts("PASS: clear");

    /* Test 8: ring around buffer multiple times */
    rb_clear(rb);
    for (int i = 0; i < 100; i++) {
        uint8_t b = (uint8_t)(i & 0xFF), rbuf;
        /* Write one byte */
        size_t w_n = rb_write(rb, &b, 1);
        if (w_n != 1) { puts("FAIL: wrap write"); return 1; }
        /* Read oldest byte when buffer is full-ish */
        if (rb_size(rb) >= 4) {
            size_t r_n = rb_read(rb, &rbuf, 1);
            if (r_n != 1 || rbuf != (uint8_t)((i - 3) & 0xFF)) {
                printf("FAIL: wrap read at %d\n", i); return 1;
            }
        }
    }
    puts("PASS: multiple wrap-around");

    rb_destroy(rb);
    puts("\nAll tests passed!");
    return 0;
}
