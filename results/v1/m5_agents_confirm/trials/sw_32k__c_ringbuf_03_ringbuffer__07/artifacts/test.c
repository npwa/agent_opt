#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int passed = 0, failed = 0;

#define check(cond, msg) do { \
    if (cond) { passed++; } else { printf("FAIL: %s\n", msg); failed++; } \
} while(0)

int main(void)
{
    /* rb_create / rb_destroy */
    ringbuf_t *rb = rb_create(16);
    check(rb != NULL, "create returns non-NULL");
    check(rb_capacity(rb) == 16, "capacity is 16");
    check(rb_size(rb) == 0, "size is 0 after create");

    /* NULL destroy */
    rb_destroy(NULL);

    /* Invalid create */
    check(rb_create(0) == NULL, "create(0) returns NULL");

    /* Write and read 4 bytes */
    uint8_t msg[] = {1, 2, 3, 4};
    size_t n = rb_write(rb, msg, sizeof(msg));
    check(n == 4, "wrote 4 bytes");
    check(rb_size(rb) == 4, "size is 4 after write");

    uint8_t buf[4];
    n = rb_read(rb, buf, sizeof(buf));
    check(n == 4, "read 4 bytes");
    check(memcmp(buf, msg, 4) == 0, "read data matches written");
    check(rb_size(rb) == 0, "size is 0 after read");

    /* Full buffer */
    for (int i = 0; i < 16; i++) {
        uint8_t v = (uint8_t)i;
        check(rb_write(rb, &v, 1) == 1, "fill byte");
    }
    check(rb_size(rb) == 16, "size is full capacity");
    check(rb_write(rb, (uint8_t[]){99}, 1) == 0, "full buffer writes 0");

    /* Read all */
    uint8_t rbuf[16];
    check(rb_read(rb, rbuf, 16) == 16, "read all from full");
    for (int i = 0; i < 16; i++)
        check(rbuf[i] == (uint8_t)i, "read sequential values");

    /* Wrap-around test: write past end, continue at start */
    rb_clear(rb);
    uint8_t w1[] = {7,8,9,10,11,12,13,14,15};
    check(rb_write(rb, w1, sizeof(w1)) == 9, "write 9 bytes");
    
    uint8_t r1[2];
    check(rb_read(rb, r1, sizeof(r1)) == 2, "read 2 leading bytes");
    check(r1[0] == 7 && r1[1] == 8, "first two values correct after partial read");

    /* Now head is at position 9, tail at 2, count=7. Free=9. */
    uint8_t w2[] = {97,98,99,100,101};
    check(rb_write(rb, w2, sizeof(w2)) == 5, "write wraps 5 bytes to end and start");
    
    /* Read all 12 (7+5) */
    uint8_t rall[12];
    n = rb_read(rb, rall, sizeof(rall));
    check(n == 12, "read all wrapped data");
    check(rall[0] == 9 && rall[1] == 10 && rall[7] == w2[0] && rall[8] == w2[1], "wrap values correct");

    /* Peek test */
    rb_clear(rb);
    uint8_t pdata[] = {42, 43, 44};
    rb_write(rb, pdata, sizeof(pdata));
    uint8_t pbuf[2];
    n = rb_peek(rb, pbuf, sizeof(pbuf));
    check(n == 2, "peek returns 2");
    check(pbuf[0] == 42 && pbuf[1] == 43, "peek first two correct");
    check(rb_size(rb) == 3, "peek does not change size");

    /* test partial write */
    rb_clear(rb);
    for (int i = 0; i < 15; i++) {
        uint8_t v = (uint8_t)i;
        rb_write(rb, &v, 1);
    }
    check(rb_size(rb) == 15, "size is 15");
    check(rb_write(rb, (uint8_t[]){99, 88}, 2) == 1, "partial write returns fitted count=1");

    rb_clear(rb);
    check(rb_size(rb) == 0, "clear empties buffer");

    /* NULL operations */
    check(rb_write(NULL, msg, 4) == 0, "write NULL rb returns 0");
    check(rb_read(NULL, buf, 4) == 0, "read NULL rb returns 0");
    check(rb_peek(NULL, buf, 4) == 0, "peek NULL rb returns 0");
    check(rb_size(NULL) == 0, "size NULL rb returns 0");
    check(rb_capacity(NULL) == 0, "capacity NULL rb returns 0");

    /* Create(0) */
    check(rb_create(0) == NULL, "create(0) returns NULL");

    /* Capacity mismatch with large write */
    ringbuf_t *rb2 = rb_create(4);
    uint8_t big[] = {1,2,3,4,5,6,7};
    check(rb_write(rb2, big, sizeof(big)) == 4, "large write writes what fits (4)");
    rb_destroy(rb2);

    rb_destroy(rb);

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
