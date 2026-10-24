#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while (0)

int main(void) {
    ringbuf_t *rb;
    uint8_t data[256], out[8];
    size_t r;

    /* --- create --destroy --- */
    rb = rb_create(4); CHECK(rb != NULL, "create capacity=4");
    CHECK(rb_capacity(rb) == 4, "capacity returns 4");
    rb_destroy(rb);

    CHECK(rb_create(0) == NULL, "create(0) returns NULL");
    CHECK(rb_create(-1) == NULL, "create overflow returns NULL");
    rb_destroy(NULL); /* should not crash */

    /* --- null safety --- */
    memset(out, 0, sizeof(out));
    CHECK(rb_write(NULL, out, 4) == 0, "write(NULL,...) = 0");
    CHECK(rb_read(NULL, out, 4) == 0, "read(NULL,...) = 0");
    CHECK(rb_peek(NULL, out, 4) == 0, "peek(NULL,...) = 0");
    CHECK(rb_size(NULL) == 0, "size(NULL) = 0");
    CHECK(rb_capacity(NULL) == 0, "capacity(NULL) = 0");
    rb_clear(NULL);

    /* --- full write -- read round trip (small buf) --- */
    rb = rb_create(8);
    for (r = 0; r < 8; r++) data[r] = (uint8_t)(10 + r);
    r = rb_write(rb, data, 8);
    CHECK(r == 8, "write 8/8 fills");

    memset(out, 0, sizeof(out));
    r = rb_read(rb, out, 5);
    CHECK(r == 5, "read returns 5 when 8 available");

    /* first 5 bytes should match */
    { int i; for (i = 0; i < 5; i++) CHECK(out[i] == (uint8_t)(10 + i), "peek byte i"); }

    CHECK(rb_size(rb) == 3, "read reduces size to 3");
    CHECK(rb_read(rb, out, 4) == 3, "read less than remaining");

    { int i; for (i = 0; i < 3; i++) CHECK(out[i] == (uint8_t)(15 + i), "tail bytes correct"); }

    rb_clear(rb);
    CHECK(rb_size(rb) == 0 && rb_capacity(rb) == 8, "clear empties but keeps capacity");

    /* --- partial write -- data that does not fit --- */
    r = rb_write(rb, (const uint8_t[]){42}, 1);
    CHECK(r == 1, "can write into empty buffer");
    r = rb_write(rb, (const uint8_t[]){43,44}, 2);
    CHECK(r == 7, "partial write: 7 of 8 available");

    /* peek -- size must still be 8 */
    memset(out, 0, sizeof(out));
    r = rb_peek(rb, out, 3);
    CHECK(r == 3, "peek returns 3");
    CHECK(out[0] == 42 && out[1] == 43 && out[2] == 44, "peek bytes match (oldest first)");
    CHECK(rb_size(rb) == 8, "peek does not reduce size");

    rb_destroy(rb);

    /* --- wrap-around test --- */
    rb = rb_create(6);
    r = rb_write(rb, (const uint8_t[]){1, 2, 3}, 3);
    CHECK(r == 3 && rb_size(rb) == 3, "write 3 bytes");

    /* drain everything */
    memset(out, 0, sizeof(out));
    r = rb_read(rb, out, 6);
    CHECK(r == 3 && out[0] == 1 && out[1] == 2 && out[2] == 3, "read drained all 3");
    CHECK(rb_size(rb) == 0, "size now zero");

    /* write again -- should go into head of buffer after head moved past old reads */
    r = rb_write(rb, (const uint8_t[]){10, 20, 30}, 3);
    CHECK(r == 3 && rb_size(rb) == 3, "write after drain: 3 bytes");

    /* fill remaining to capacity then more */
    r = rb_write(rb, (const uint8_t[]){40,50,60,70}, 4);
    CHECK(r == 3, "overflow write fits 3 of 4 into size-remaining");
    CHECK(rb_size(rb) == 6, "now full at capacity");

    /* read back all */
    { CHECK(rb_read(rb, out, 6) == 6, "read 6 from full buf");
      /* oldest: 10,20,30 from second write, then first 3 of second overflow write */
      CHECK(out[0] == 10 && out[1] == 20 && out[2] == 30, "bytes 0-2 after wrap");
      CHECK(out[3] == 40 && out[4] == 50 && out[5] == 60, "bytes 3-5 after wrap");
    }

    rb_destroy(rb);

    /* --- summary --- */
    if (failures) { printf("FAILURES: %d\n", failures); return 1; }
    printf("ALL TESTS PASSED\n");
    return 0;
}
