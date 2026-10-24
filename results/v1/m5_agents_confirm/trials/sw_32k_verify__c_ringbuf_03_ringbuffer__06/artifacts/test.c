#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int passed = 0, failed = 0;

#define CHECK(cond, fmt, ...) do {                       \
    if (cond) { ++passed; printf("  PASS: " fmt "\n", ##__VA_ARGS__); } \
    else      { ++failed;     printf("FAIL: " fmt "\n", ##__VA_ARGS__); } \
} while (0)

int main(void)
{
    /* ---- NULL / edge cases ---- */
    printf("[NULL / edge cases]\n");
    CHECK(rb_create(0) == NULL,                     "create(cap=0) returns NULL");
    rb_destroy(NULL);                              /* should not crash */
    rb_clear(NULL);                                /* should not crash */
    CHECK(rb_write(NULL, (uint8_t[]){1}, 1) == 0, "write(NULL) -> 0");
    CHECK(rb_read(NULL, NULL, 0)    == 0,          "read(NULL,_,0) -> 0");
    CHECK(rb_peek(NULL, NULL, 0)    == 0,          "peek(NULL,_,0) -> 0");
    CHECK(rb_size(NULL)             == 0,          "size(NULL) -> 0");
    CHECK(rb_capacity(NULL)         == 0,          "capacity(NULL) -> 0");

    /* ---- basic create / capacity ---- */
    printf("\n[create / capacity]\n");
    ringbuf_t *rb = rb_create(4);
    CHECK(rb != NULL,                          "create(cap=4) succeeds");
    CHECK(rb_capacity(rb) == 4,                "capacity is 4");

    /* ---- round-trip: full write then read ---- */
    printf("\n[round-trip]\n");
    uint8_t wdata[] = {0xA1, 0xB2, 0xC3, 0xD4};
    size_t n = rb_write(rb, wdata, 4);
    CHECK(n == 4,                              "wrote 4 bytes into full buffer");
    CHECK(rb_size(rb) == 4,                    "size is 4 after full write");

    uint8_t rdata[4];
    n = rb_read(rb, rdata, 4);
    CHECK(n == 4,                              "read 4 bytes out of full buffer");
    CHECK(memcmp(rdata, wdata, 4) == 0,        "read data matches written data");
    CHECK(rb_size(rb) == 0,                    "size is 0 after drain");

    /* ---- wrap-around: write-partially -> read-some -> write-more (wraps) ---- */
    printf("\n[wrap-around]\n");
    uint8_t tmp[16];

    rb_write(rb, (uint8_t[]){10, 20}, 2);     /* head advances to index 2   */
    CHECK(rb_size(rb) == 2,                     "wrote 2 bytes");

    n = rb_read(rb, tmp, 1);                    /* remove oldest byte       */
    CHECK(n == 1 && tmp[0] == 10,               "read oldest byte = 10");

    uint8_t more[] = {30, 40, 50, 60};
    n = rb_write(rb, more, 4);                  /* free = 4-1 = 3; writes=3 */
    CHECK(n == 3,                                "partial write returned count");

    /* ringbuf is now full (cap=4, wrote 1+3=4 bytes). Read them all back: */
    n = rb_read(rb, tmp, 8);
    CHECK(n == (size_t)4,                        "read back 4 remaining bytes");
    CHECK(tmp[0] == 20 && tmp[1] == 30 && tmp[2] == 40 && tmp[3] == 50,
                                        "order: [20,30,40,50]");

    /* ==== Clean state for subsequent tests === */
    rb_clear(rb);                                 /* ensure clean slate      */

    /* ---- peek (non-destructive) ---- */
    printf("\n[peek]\n");
    ringbuf_t *rb2 = rb_create(8);
    rb_write(rb2, (uint8_t[]){7, 8, 9}, 3);
    uint8_t p[4];
    n = rb_peek(rb2, p, 3);
    CHECK(n == 3 && p[0] == 7 && p[1] == 8 && p[2] == 9, "peek sees oldest 3 bytes");
    CHECK(rb_size(rb2) == 3,                    "peek doesn't change size");

    /* ---- clear ---- */
    printf("\n[clear]\n");
    rb_write(rb2, (uint8_t[]){1, 2, 3, 4}, 4); /* now size is 3+4=7 */
    CHECK(rb_size(rb2) == 7,                    "size=7 after more writes");
    rb_clear(rb2);
    CHECK(rb_size(rb2) == 0,                    "size=0 after clear");
    CHECK(rb_capacity(rb2) == 8,                "capacity unchanged by clear");
    /* buffer is reusable after clear */
    uint8_t x[] = {99};
    n = rb_write(rb2, x, 1);
    CHECK(n == 1 && rb_size(rb2) == 1,         "reusable after clear");

    /* ---- write more than free space (size==1, cap==8 => free==7) ---- */
    printf("\n[partial write]\n");
    uint8_t big[] = {0,1,2,3,4};
    n = rb_write(rb2, big, 5);                  /* can store all 5 (free=7) */
    CHECK(n == 5,                                "full write fits when enough room");
    CHECK(rb_size(rb2) == 6,                    "size is 1+5=6");

    /* ---- drain to zero then clear buffer for next test ---- */
    rb_read(rb2, tmp, 8);
    rb_clear(rb2);

    /* ==== Use a small, carefully-accounted-for ringbuf for final tests === */
    printf("\n[small-wrap]\n");
    ringbuf_t *rb3 = rb_create(3);             /* tiny buffer */
    CHECK(rb_capacity(rb3) == 3,                "cap=3");

    /* fill it up completely */
    n = rb_write(rb3, (uint8_t[]){'A', 'B', 'C'}, 3);
    CHECK(n == 3,                               "filled small buf");

    /* try to write more — should store 0 because full */
    n = rb_write(rb3, (uint8_t[]){'D'}, 1);
    CHECK(n == 0,                               "write to full buffer returns 0");

    /* read one out: [A] removed; free space = 1; ring: [B,C]          */
    uint8_t a_out;
    n = rb_read(rb3, &a_out, 1);
    CHECK(n == 1 && a_out == 'A',               "read A out");

    /* write one more in — wraps around end of storage                */
    uint8_t dd = 'D';
    n = rb_write(rb3, &dd, 1);
    CHECK(n == 1,                               "write D after partial drain");

    /* read all three: [B,C,D]                                                */
    size_t nr = rb_read(rb3, tmp, 4);
    CHECK(nr == 3,                              "read B,C,D back out");
    CHECK(tmp[0] == 'B' && tmp[1] == 'C' && tmp[2] == 'D',
                                                    "order: [B,C,D]");

    /* peek on empty                                                   */
    n = rb_peek(rb3, p, 4);
    CHECK(n == 0,                               "peek on empty returns 0");

    /* clear then verify capacity still OK                           */
    rb_clear(rb3);
    CHECK(rb_capacity(rb3) == 3,                "cap survives clear+drain");

    rb_destroy(rb);
    rb_destroy(rb2);
    rb_destroy(rb3);
    rb_destroy(NULL);                            /* idempotent destroy check */

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
