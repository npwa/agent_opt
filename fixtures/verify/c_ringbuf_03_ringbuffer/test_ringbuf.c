/* Hidden verifier for c_ringbuf_03_ringbuffer: includes the agent's ringbuf.c. */
#include <stdio.h>
#include <string.h>

#include "ringbuf.c"

static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void) {
    uint8_t out[64];

    CHECK(rb_create(0) == NULL);
    rb_destroy(NULL);

    ringbuf_t *rb = rb_create(8);
    CHECK(rb != NULL);
    CHECK(rb_capacity(rb) == 8 && rb_size(rb) == 0);
    CHECK(rb_read(rb, out, 4) == 0);

    CHECK(rb_write(rb, (const uint8_t *)"abcdef", 6) == 6);
    CHECK(rb_size(rb) == 6);
    CHECK(rb_peek(rb, out, 3) == 3 && memcmp(out, "abc", 3) == 0);
    CHECK(rb_size(rb) == 6);
    CHECK(rb_read(rb, out, 4) == 4 && memcmp(out, "abcd", 4) == 0);

    /* wrap-around: head is at 4, write 6 more -> only 6 free */
    CHECK(rb_write(rb, (const uint8_t *)"ghijkl", 6) == 6);
    CHECK(rb_size(rb) == 8);
    CHECK(rb_write(rb, (const uint8_t *)"Z", 1) == 0);
    CHECK(rb_read(rb, out, 8) == 8 && memcmp(out, "efghijkl", 8) == 0);
    CHECK(rb_size(rb) == 0);

    /* partial write when too large */
    CHECK(rb_write(rb, (const uint8_t *)"0123456789", 10) == 8);
    CHECK(rb_read(rb, out, 3) == 3 && memcmp(out, "012", 3) == 0);
    CHECK(rb_write(rb, (const uint8_t *)"AB", 2) == 2);
    CHECK(rb_peek(rb, out, 64) == 7 && memcmp(out, "34567AB", 7) == 0);

    rb_clear(rb);
    CHECK(rb_size(rb) == 0 && rb_read(rb, out, 1) == 0);

    /* NULL handling */
    CHECK(rb_write(rb, NULL, 3) == 0);
    CHECK(rb_read(rb, NULL, 3) == 0);
    CHECK(rb_write(NULL, (const uint8_t *)"a", 1) == 0);

    /* many wrap cycles */
    for (int i = 0; i < 1000; i++) {
        uint8_t b[5] = { (uint8_t)i, (uint8_t)(i + 1), (uint8_t)(i + 2), (uint8_t)(i + 3), (uint8_t)(i + 4) };
        CHECK(rb_write(rb, b, 5) == 5);
        CHECK(rb_read(rb, out, 5) == 5 && memcmp(out, b, 5) == 0);
    }
    rb_destroy(rb);

    /* capacity 1 */
    rb = rb_create(1);
    CHECK(rb_write(rb, (const uint8_t *)"xy", 2) == 1);
    CHECK(rb_read(rb, out, 2) == 1 && out[0] == 'x');
    rb_destroy(rb);

    return fails ? 1 : 0;
}
