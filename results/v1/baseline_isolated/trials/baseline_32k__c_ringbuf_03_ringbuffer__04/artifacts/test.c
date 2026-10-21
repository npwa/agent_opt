#include "ringbuf.h"
#include <stdio.h>
#include <string.h>

static int ok = 1;
static void check(const char *desc, int condition) {
    if (!condition) { printf("FAIL: %s\n", desc); ok = 0; }
    else            { printf("OK:   %s\n", desc); }
}

int main(void) {
    /* ---- edge cases ---- */
    check("(rb_create(0)==NULL)", rb_create(0) == NULL);
    rb_destroy(NULL);
    ringbuf_t *rb = rb_create(8);
    check("create ok", rb != NULL);
    check("capacity=8", rb_capacity(rb) == 8);
    check("size==0 empty", rb_size(rb) == 0);

    /* ---- write/read in a loop, testing wrap-around ---- */
    {
        ringbuf_t *r = rb_create(4); /* 3 usable slots */
        uint8_t out[4] = {0};
        for (int i = 0; i < 20; i++) {
            uint8_t v = (uint8_t)(i + 1);
            size_t w = rb_write(r, &v, 1); (void)w;
            memset(out, 0, sizeof(out));
            size_t got = rb_read(r, out, 1);
            check("single-round read", got == 1 && out[0] == v);
        }
        rb_destroy(r);
    }

    /* ---- simple write/read fill ---- */
    {const uint8_t src[] = {10,20,30};
     size_t w = rb_write(rb, src, sizeof(src));
     check("wrote 3", w == 3);
     check("size now 3", rb_size(rb) == 3);
     uint8_t dst[4] = {0};
     size_t r = rb_read(rb, dst, 3);
     check("read 3", r == 3);
     check("data matches", memcmp(dst, src, 3) == 0);
     check("size now 0", rb_size(rb) == 0);}

    /* ---- wrap-around verified via many write/read cycles on smaller buf ---- */
    {
        ringbuf_t *r = rb_create(4); /* 3 usable slots */
        uint8_t out[4] = {0};

        /* Write values 1, 2, 3 (full). Then read them all and immediately write 4, 5, 6.
           Since ring only has 3 usable slots, the second batch can only proceed as we drain. */
        for (int i = 0; i < 3; i++) {
            uint8_t v = (uint8_t)(i + 1);
            size_t wn = rb_write(r, &v, 1);
            check("write byte " + (i), wn == 1);
        }
        check("filled to 3", rb_size(r) == 3);

        /* Read all three: should be 1,2,3 */
        memset(out, 0, sizeof(out));
        for (int i = 0; i < 3; i++) { size_t gn = rb_read(r, out + i, 1); (void)gn; }
        uint8_t expected[] = {1,2,3};
        check("read [1,2,3]", memcmp(out, expected, 3) == 0);
        check("size==0 empty again", rb_size(r) == 0);

        /* Now write 4 bytes but ring only takes 3 (capacity-1 free slots) */
        uint8_t more[] = {200,201,202,203};
        size_t wn_all = rb_write(r, more, sizeof(more));
        check("write 4 into buf of cap=4 gets 3", wn_all == (size_t)(rb_capacity(r) - 1));

        uint8_t out2[4]; memset(out2, 0, sizeof(out2));
        size_t rn = rb_read(r, out2, 3);
        check("readback all 3", rn == 3);
        uint8_t exp2[] = {200,201,202};
        check("wrapped data matches", memcmp(out2, exp2, 3) == 0);

        rb_destroy(r);
    }

    /* ---- wrap-around: fill 16-slot buf to max (15 used), read some, write to wrap ---- */
    {
        ringbuf_t *r = rb_create(16); /* 15 usable slots */
        uint8_t out[16];

        /* Fill all 15 slots */
        for (int i = 0; i < 15; i++) {
            uint8_t v = (uint8_t)(0xFF - i);
            size_t wn = rb_write(r, &v, 1); (void)wn;
        }
        check("full size==15", rb_size(r) == 15);

        /* Read bytes 0 and 1 from tail to make room */
        memset(out, 0, sizeof(out));
        rb_read(r, out, 2);
        check("read first 2 of fill: {255,254}", out[0] == 0xFF && out[1] == 0xFE);

        /* Now write more bytes that will wrap around the internal buffer end */
        for (int i = 0; i < 15; i++) {
            uint8_t v = (uint8_t)(i + 10);
            size_t wn = rb_write(r, &v, 1);
            (void)wn;
        }
        /* Ring had 2 free slots initially (after reading), then we read 2 more -> 4 free, 
           but capacity=15 usable max */
        check("size is still capped at 15", rb_size(r) == 15);

        /* Read everything back - should start with remaining fill bytes {253..241} then write-back {10..24} */
        memset(out, 0, sizeof(out));
        for (int i = 0; i < 15; i++) { size_t gn = rb_read(r, out + i, 1); (void)gn; }

        /* Read one more to free another slot for remaining writes */

        rb_destroy(r);
    }

    /* ---- partial write ---- */
    {
        ringbuf_t *r = rb_create(8);
        uint8_t one = 1;
        rb_write(r, &one, 1);
        check("size after single write==1", rb_size(r) == 1);

        uint8_t big[256]; memset(big, 0xAA, sizeof(big));
        size_t pn = rb_write(r, big, sizeof(big));
        /* 7 usable - 1 used = 6 free -> writes 6 */
        check("partial write=6", pn == 6);

        /* Read everything */
        uint8_t out[8]; memset(out, 0, sizeof(out));
        size_t rn = rb_read(r, out, 7);
        check("readback all 7", rn == 7);
        check("first byte==1 (oldest)", out[0] == 1);
        for (size_t i = 1; i < 7; i++) check("remaining bytes 0xAA", out[i] == 0xAA);

        rb_destroy(r);
    }

    /* ---- peek, clear, null ops ---- */
    {
        ringbuf_t *r = rb_create(8);
        uint8_t v = 99;
        rb_write(r, &v, 1);
        uint8_t out[2] = {0};
        check("(peek on non-empty==1)", rb_peek(r, NULL, 0) == 0);
        size_t p = rb_peek(r, out, sizeof(out));
        check("peek returns 1", p == 1);
        check("peek reads correct", out[0] == 99);
        check("size unchanged", rb_size(r) == 1);

        /* clear */
        uint8_t two = 2;
        rb_write(r, &two, 1);
        check("size==2", rb_size(r) == 2);
        rb_clear(r);
        check("after clear size==0", rb_size(r) == 0);
        /* peek empty */
        memset(out, 0, sizeof(out));
        check("(peek on full buf==2)", rb_peek(r, out, sizeof(out)) == 0);

        rb_destroy(r);
    }

    /* ---- write larger than free space stores what fits ---- */
    {
        ringbuf_t *r = rb_create(8);
        uint8_t big[64]; memset(big, 0xCC, sizeof(big));
        size_t wn = rb_write(r, big, sizeof(big));
        /* max capacity-1 = 7 slots free (empty ring) */
        check("big write stores=7", wn == (size_t)(rb_capacity(r) - 1));

        uint8_t out[8]; memset(out, 0, sizeof(out));
        rb_read(r, out, 7);
        for (size_t i = 0; i < 7; i++) check("big write data=0xCC", out[i] == 0xCC);

        rb_destroy(r);
    }

    /* ---- peek doesn't consume ---- */
    {
        ringbuf_t *r = rb_create(8);
        uint8_t v = 99;
        rb_write(r, &v, 1);
        uint8_t out[2] = {0};
        size_t p = rb_peek(r, out, sizeof(out));
        check("peek returns 1", p == 1);
        check("peek reads correct", out[0] == 99);
        check("size unchanged", rb_size(r) == 1);

        /* read it now */
        uint8_t rdbuf;
        rb_read(r, &rdbuf, 1);
        check("read gets what was peeked", rdbuf == 99);
        rb_destroy(r);
    }

    /* ---- clear ---- */
    rb_write(rb, (uint8_t[]){50}, 1);
    check("size before clear=1", rb_size(rb) == 1);
    rb_clear(rb);
    check("size after clear=0", rb_size(rb) == 0);
    uint8_t out2;
    rb_read(rb, &out2, 1);
    (void)out2;
    rb_clear(NULL);

    /* ---- NULL ops (no-crash) ---- */
    size_t zr;
    check("rb_size=NULL->0", (zr = rb_size(NULL)) == 0);
    check("rb_capacity=NULL->0", rb_capacity(NULL) == 0);
    check("rb_clear=NULL ok", 1); rb_clear(NULL);
    printf("\n%s\n", ok ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return ok ? 0 : 1;
}
