#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int pass = 0, fail = 0;

#define check(cond) do { pass++; if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while(0)

int main(void)
{
    /* === construct / destroy / NULL-safety === */
    check(rb_create(0) == NULL);
    ringbuf_t *rb = rb_create(8);
    if (!rb) { printf("FATAL: rb_create(8) returned NULL\n"); return 1; }
    check(rb_capacity(rb) == 8);
    check(rb_size(rb) == 0);

    rb_destroy(NULL);  /* must not crash */

    /* === simple write + read round-trip === */
    uint8_t msg[] = {1,2,3,4,5};
    check(rb_write(rb, msg, 5) == 5);
    check(rb_size(rb) == 5);

    uint8_t buf[8];
    check(rb_read(rb, buf, 8) == 5);   /* can pass larger out */
    check(memcmp(buf, msg, 5) == 0);
    check(rb_size(rb) == 0);

    /* === write to nearly-full buffer (partial fill) === */
    uint8_t m1[] = {10,20,30,40,50,60,70};  /* 7 bytes */
    check(rb_write(rb, m1, 7) == 7);       /* fills 7 of 8 */
    check(rb_size(rb) == 7);

    /* 1 slot left — write request for 4 bytes but only 1 should stick */
    uint8_t m2[] = {99};
    size_t written = rb_write(rb, m2, 4);  /* wants 4 bytes but only 1 free */
    check(written == 1);
    check(rb_size(rb) == 8);               /* buffer now full */

    /* write on a FULL buffer returns 0 */
    check(rb_write(rb, m1, 3) == 0);

    /* read everything back and verify order */
    uint8_t out[16];
    uint8_t expected[] = {10,20,30,40,50,60,70,99};
    size_t rd = rb_read(rb, out, 16);
    check(rd == 8);
    check(memcmp(out, expected, 8) == 0);

    /* === wrap-around write + read === */
    rb_clear(rb);
    /* Drain everything then fill from midpoint so we test wrap-around */
    uint8_t right[] = {100,101,102,103};  /* 4 bytes */
    rb_write(rb, right, sizeof(right));
    uint8_t tmp[4];
    memset(tmp, 0, sizeof(tmp));
    rb_read(rb, tmp, 4);           /* size=0, head=tail=4 */

    /* Write 8 bytes starting after drain (tail=4) — must wrap at capacity boundary */
    uint8_t many[] = {3,4,5,6,7,8,9,10};   /* 8 bytes */
    check(rb_write(rb, many, sizeof(many)) == 8);  /* exactly fills capacity */

    /* Verify the wrapped content: data is in original order after read-back */
    check(rb_read(rb, out, 8) == 8);
    uint8_t want[] = {3,4,5,6,7,8,9,10};
    check(memcmp(out, want, 8) == 0);

    /* === peek (does not remove data) === */
    rb_clear(rb);
    rb_write(rb, msg, 5);
    uint8_t peeked[3];
    size_t pn = rb_peek(rb, peeked, 3);
    check(pn == 3);
    check(memcmp(peeked, msg, 3) == 0);
    check(rb_size(rb) == 5);       /* peek must not change size */

    /* === capacity-1 edge case === */
    rb_destroy(rb);
    ringbuf_t *tiny = rb_create(1);
    if (!tiny) { printf("FATAL: rb_create(1) returned NULL\n"); return 1; }
    uint8_t single = 42;
    check(rb_write(tiny, &single, 1) == 1);
    check(rb_size(tiny) == 1);
    check(rb_read(tiny, buf, 1) == 1);
    check(buf[0] == 42);

    /* === capacity-2 wrap-around test === */
    rb_destroy(tiny);
    ringbuf_t *tiny2 = rb_create(2);
    if (!tiny2) { printf("FATAL: rb_create(2) returned NULL\n"); return 1; }

    /* Write 2 bytes: positions 0->1, then 1->wrap to 0 */
    uint8_t ab[] = {0xAA, 0xBB};
    check(rb_write(tiny2, ab, 2) == 2);
    check(rb_size(tiny2) == 2);

    /* Drain and read back */
    uint8_t rbuf[4];
    size_t rr = rb_read(tiny2, rbuf, 4);
    check(rr == 2);
    uint8_t want_ab[] = {0xAA, 0xBB};
    check(memcmp(rbuf, want_ab, 2) == 0);

    /* Peek on NULL / empty */
    check(rb_peek(NULL, buf, 1) == 0);
    check(rb_size(NULL) == 0);
    rb_clear(NULL);              /* must not crash */

    /* === large capacity test (stress wrap-around) === */
    ringbuf_t *big = rb_create(256);
    if (!big) { printf("FATAL: rb_create(256) returned NULL\n"); return 1; }
    uint8_t payload[300];      /* more than capacity, tests partial write */
    for (int i = 0; i < 300; i++) payload[i] = (uint8_t)i;
    check(rb_write(big, payload, sizeof(payload)) == 256);

    uint8_t readback[256];
    size_t rn = rb_read(big, readback, sizeof(readback));
    check(rn == 256);
    for (int i = 0; i < 256; i++)
        check(readback[i] == (uint8_t)i);

    /* Multiple wrap cycles */
    rb_clear(big);
    uint8_t chunk_a[] = {1,2,3};       /* write at tail=0, positions 0-2 */
    check(rb_write(big, chunk_a, sizeof(chunk_a)) == 3);
    uint8_t d[4]; int dcnt = (int)rb_read(big, d, sizeof(d));
    check(dcnt == 3);

    /* tail is now 3. Write 200 bytes that wrap past index 255 back to index 0 */
    uint8_t chunk_b[200];
    for (int i = 0; i < 200; i++) chunk_b[i] = (uint8_t)(i + 10);
    check(rb_write(big, chunk_b, sizeof(chunk_b)) == 200);

    /* Read back and verify the wrapped portion (200 bytes are stored) */
    uint8_t rb_buf[256];
    rn = (size_t)rb_read(big, rb_buf, sizeof(rb_buf));
    check(rn == 200);

    /* The read bytes must match chunk_b in order: first byte should be chunk_b's value */
    check(rb_buf[0] == 10);   /* chunk_b[0] = 10 */
    check(rb_buf[199] == 209); /* chunk_b[199] = 209 (10+199) */

    rb_destroy(big);

    /* === capacity-1 full drain then write again === */
    ringbuf_t *one = rb_create(1);
    uint8_t v;
    for (int loop = 0; loop < 3; loop++) {
        uint8_t wdata = (uint8_t)(loop * 10 + 5);
        check(rb_write(one, &wdata, 1) == 1);
        check(rb_read(one, &v, 1)       == 1);
        check(v == wdata);
    }

    rb_destroy(one);

    /* === Summary === */
    printf("PASS %d  FAIL %d\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
