#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

#define EXPECT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while(0)

int main(void)
{
    /* rb_create / rb_capacity / rb_size */
    ringbuf_t *rb = rb_create(8);
    EXPECT(rb != NULL, "create");
    EXPECT(rb_capacity(rb) == 8, "capacity");
    EXPECT(rb_size(rb) == 0, "empty size");

    /* basic write + read */
    const char txt[] = "Hello";
    size_t n = rb_write(rb, (const uint8_t *)txt, strlen(txt));
    EXPECT(n == 5, "write count");
    EXPECT(rb_size(rb) == 5, "size after write");

    uint8_t buf[256];
    n = rb_read(rb, buf, sizeof(buf));
    EXPECT(n == 5, "read count");
    EXPECT(memcmp(buf, "Hello", 5) == 0, "data match");
    EXPECT(rb_size(rb) == 0, "size after read");

    /* wrap-around: fill, read some, then write wraps into freed space */
    n = rb_write(rb, (const uint8_t *)"12345678", 8);
    EXPECT(n == 8, "fill 8");
    n = rb_read(rb, buf, 5);
    EXPECT(n == 5, "read 5");
    /* head@5 tail@2 sz=3; free=5 so all 4 of "ABCD" fit, writing at [5] wraps to [6][7][0][1] */
    n = rb_write(rb, (const uint8_t *)"ABCD", 4);
    EXPECT(n == 4, "wrap-around write fits all 4");
    EXPECT(rb_size(rb) == 7, "size after partial write");

    /* peek: oldest is tail@2 -> buffer has 3 old + ABCD new = 7 bytes */
    n = rb_peek(rb, buf, 256);
    EXPECT(n == 7, "peek count");

    /* clear */
    rb_clear(rb);
    EXPECT(rb_size(rb) == 0, "size after clear");
    EXPECT(rb_capacity(rb) == 8, "capacity after clear");

    /* partial-wrap: fill to capacity then read some, write more than free */
    ringbuf_t *rb3 = rb_create(4);
    n = rb_write(rb3, (const uint8_t *)"1234", 4);
    EXPECT(n == 4, "fill full");
    n = rb_read(rb3, buf, 2);
    EXPECT(n == 2 && memcmp(buf, "12", 2) == 0, "read first two");
    /* now sz=2, free=2. write 5 but only 2 fit */
    n = rb_write(rb3, (const uint8_t *)"ABCDE", 5);
    EXPECT(n == 2, "partial wrap write fits 2");
    rb_destroy(rb3);

    /* NULL safety */
    EXPECT(rb_write(NULL, (uint8_t *)"x", 1) == 0, "NULL write");
    EXPECT(rb_read(NULL, buf, 1) == 0, "NULL read");
    EXPECT(rb_peek(NULL, buf, 1) == 0, "NULL peek");
    EXPECT(rb_size(NULL) == 0, "NULL size");
    EXPECT(rb_capacity(NULL) == 0, "NULL capacity");
    rb_destroy(NULL); /* should not crash */

    /* create(0) invalid */
    EXPECT(rb_create(0) == NULL, "create zero cap");

    /* exact fit + wrap-around full cycle */
    ringbuf_t *rb2 = rb_create(4);
    rb_write(rb2, (uint8_t *)"ABCDEFGH", 8); /* should write 4 */
    uint8_t b2[256];
    size_t m = rb_read(rb2, b2, 8);
    EXPECT(m == 4, "wrapped read");
    EXPECT(memcmp(b2, "ABCD", 4) == 0, "wrapped data");
    rb_destroy(rb2);

    printf("ALL TESTS PASSED\n");
    return 0;
}
