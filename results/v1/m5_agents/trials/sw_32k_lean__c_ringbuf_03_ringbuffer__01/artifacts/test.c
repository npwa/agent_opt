#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    /* --- rb_create / rb_capacity / rb_size --- */
    ringbuf_t *rb = rb_create(8);
    CHECK(rb != NULL, "create");
    CHECK(rb_capacity(rb) == 8, "capacity");

    /* --- rb_write fills buffer --- */
    const uint8_t hello[] = {'h','e','l','l','o'};
    size_t n = rb_write(rb, hello, sizeof(hello));
    CHECK(n == 5, "write 5 bytes");
    CHECK(rb_size(rb) == 5, "size after write");

    /* --- rb_read returns oldest first --- */
    uint8_t buf[6];
    n = rb_read(rb, buf, 6);        /* request more than stored */
    CHECK(n == 5, "read all");
    CHECK(memcmp(buf, hello, 5) == 0, "data match");

    /* buffer now empty */
    CHECK(rb_size(rb) == 0, "size after read");

    /* --- rb_write again (tail/head reset to 0 after clear of size=0) --- */
    const uint8_t world[] = {'w','o','r','l','d'};
    n = rb_write(rb, world, sizeof(world));
    CHECK(n == 5, "write2");

    /* --- wrap-around write --- */
    memset(buf, 0, sizeof(buf));
    size_t cap = rb_capacity(rb);   /* 8 */
    size_t remaining = cap - rb_size(rb); /* 3 */

    /* Write enough to force wrap: fill rest, then some more */
    uint8_t extra[10];
    memcpy(extra, world + 5, 0);   /* just padding */
    memset(extra, 'A', sizeof(extra));
    n = rb_write(rb, extra, sizeof(extra));
    CHECK(n == remaining, "write with wrap: wrote exactly what fit");

    /* --- peek returns oldest without consuming --- */
    size_t ps = rb_peek(rb, buf, 1);
    CHECK(ps == 1, "peek count");
    CHECK(buf[0] == 'w', "peek data");
    CHECK(rb_size(rb) == cap, "size unchanged after peek");

    /* --- clear --- */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear size");
    CHECK(rb_capacity(rb) == 8, "capacity after clear");

    /* --- round-trip: fill then drain with wrap --- */
    uint8_t pattern[12];
    for (size_t i = 0; i < 12; ++i) pattern[i] = (uint8_t)(i + 1);

    n = rb_write(rb, pattern, 8);       /* fill to full */
    CHECK(n == 8, "fill full");
    n = rb_write(rb, pattern, 4);       /* should write 0 */
    CHECK(n == 0, "write when full");

    uint8_t drain[12];
    n = rb_read(rb, drain, 12);         /* read all 8 that are stored */
    CHECK(n == 8, "read all filled");
    CHECK(memcmp(drain, pattern, 8) == 0, "drain matches write");

    /* --- NULL safety --- */
    CHECK(rb_write(NULL, hello, 5) == 0, "wNULL");
    CHECK(rb_read(NULL, buf, 6) == 0, "rNULL");
    CHECK(rb_peek(NULL, buf, 1) == 0, "pNULL");
    CHECK(rb_size(NULL) == 0, "sNULL");
    CHECK(rb_capacity(NULL) == 0, "cNULL");
    rb_destroy(NULL);                   /* must not crash */

    rb_destroy(rb);

    /* --- create with capacity==0 fails --- */
    CHECK(rb_create(0) == NULL, "zero cap");

    printf("All tests passed.\n");
    return 0;
}
