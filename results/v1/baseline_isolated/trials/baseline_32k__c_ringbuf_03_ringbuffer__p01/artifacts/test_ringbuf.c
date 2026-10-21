#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ringbuf.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, cond) do { \
    if (cond) { printf("  PASS: %s\n", name); tests_passed++; } \
    else      { printf("  FAIL: %s\n", name);     tests_failed++; } \
} while(0)

int main(void)
{
    ringbuf_t *rb;
    uint8_t out[64];
    size_t n;

    /* --- create / destroy --- */
    printf("Test rb_create/destroy:\n");
    rb = rb_create(32);
    TEST("non-NULL handle", rb != NULL);
    TEST("capacity is 32", rb_capacity(rb) == 32);
    TEST("initial size is 0", rb_size(rb) == 0);

    rb_destroy(NULL);   /* allowed */
    TEST("create with 0 fails", rb_create(0) == NULL);
    rb_destroy(rb);

    /* --- write / read (normal) --- */
    printf("\nTest write/read:\n");
    rb = rb_create(16);
    n = rb_write(rb, (uint8_t *)"Hello", 5);
    TEST("wrote 5 bytes", n == 5);
    TEST("size is 5", rb_size(rb) == 5);

    memset(out, 0, sizeof(out));
    n = rb_read(rb, out, 63);
    TEST("read 5 bytes", n == 5);
    TEST("content matches", memcmp(out, "Hello", 5) == 0);
    TEST("size is 0 now", rb_size(rb) == 0);

    /* --- peek --- */
    printf("\nTest peek:\n");
    rb_write(rb, (uint8_t *)"Hello", 5);
    n = rb_peek(rb, out, 3);
    TEST("peeked 3 bytes", n == 3);
    TEST("peek content OK", memcmp(out, "Hel", 3) == 0);
    TEST("size unchanged after peek", rb_size(rb) == 5);

    /* --- wrap-around --- */
    printf("\nTest wrap-around:\n");
    rb_clear(rb);

    n = rb_write(rb, (uint8_t *)"XXXX", 4);
    TEST("wrote XXXX", n == 4);
    TEST("size is 4", rb_size(rb) == 4);

    n = rb_read(rb, out, 4);
    TEST("read XXXX", n == 4 && memcmp(out, "XXXX", 4) == 0);
    TEST("now empty", rb_size(rb) == 0);

    /* write past end boundary */
    n = rb_write(rb, (uint8_t *)"WWWWZZZZ", 8);
    TEST("wrote all 8 bytes wrapping", n == 8);
    TEST("size is 8", rb_size(rb) == 8);

    n = rb_read(rb, out, 4);
    TEST("read WWWW", n == 4 && memcmp(out, "WWWW", 4) == 0);
    TEST("size is 4 now", rb_size(rb) == 4);

    n = rb_write(rb, (uint8_t *)"AAAA1234", 8);
    TEST("wrote 8 more wrapping", n == 8);
    TEST("size is 12", rb_size(rb) == 12);

    /* read all and verify order across wrap */
    memset(out, 0, sizeof(out));
    n = rb_read(rb, out, 63);
    /* buf[4..7]=WWWW (left on buffer from earlier write) -> no, that was overwritten.
       Let's trace:
       - capacity=16, head=0,tail=0,data_len=0 after rb_clear
       - write(XXXX,4): buf[0..3]=XXXX, tail=4, head=0, data_len=4
       - read(4): head=4, returns XXXX, data_len=0.
       - write(WWWWZZZZ,8): tail=4, to_end=12, n=8<=12 -> buf[4..11]=WWWWZZZZ, tail=12, data_len=8
       - read(4): head goes 4->8, returns WWWW (buf[4..7]), data_len=4
       - write(AAAA1234,8): tail=12, to_end=4, n=8. first to_end=4: buf[12..15]=AAAA, remain=4: buf[0..3]=1234. tail=(12+8)%16=4. data_len=12
       - read all 12: starts at head=8. seq: buf[8..11]=ZZZZ, buf[12..15]=AAAA, buf[0..3]=1234 = "ZZZZAAAA1234"
    */
    TEST("wrapped order OK", n == 12 && memcmp(out, "ZZZZAAAA1234", 12) == 0);

    /* --- clear / edge --- */
    printf("\nTest edge cases:\n");
    n = rb_write(NULL, (uint8_t *)"X", 5); TEST("write(NULL)=0", n == 0);
    n = rb_read(NULL, out, 5);      TEST("read(NULL)=0", n == 0);
    n = rb_peek(NULL, out, 5);     TEST("peek(NULL)=0", n == 0);
    TEST("rb_size(NULL)=0", rb_size(NULL) == 0);

    /* full buffer */
    rb_clear(rb);
    n = rb_write(rb, (uint8_t *)"1234567890ABCDEF", 16);
    TEST("filled exactly", n == 16 && rb_size(rb) == 16);
    n = rb_write(rb, (uint8_t *)"X", 1);
    TEST("nothing fits when full", n == 0);

    /* --- summary --- */
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    rb_destroy(rb);
    return tests_failed > 0 ? 1 : 0;
}
