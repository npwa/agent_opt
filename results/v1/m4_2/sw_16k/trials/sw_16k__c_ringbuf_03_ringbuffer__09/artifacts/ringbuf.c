#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t sz;
    size_t wp;
    size_t rp;
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    rb->buf = malloc(capacity);
    if (!rb->buf) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->sz = rb->wp = rb->rp = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    size_t n = len < rb->capacity - rb->sz ? len : rb->capacity - rb->sz;
    if (n == 0) return 0;

    size_t first = n < rb->capacity - rb->wp ? n : rb->capacity - rb->wp;
    memcpy(rb->buf + rb->wp, data,        first);
    rb->wp = (rb->wp + first) % rb->capacity;
    if (n > first) {
        memcpy(rb->buf,            &data[first], n - first);
        rb->wp = (rb->wp + n - first) % rb->capacity;
    }

    rb->sz += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0 || rb->sz == 0) return 0;
    size_t n = len < rb->sz ? len : rb->sz;

    size_t first = n < rb->capacity - rb->rp ? n : rb->capacity - rb->rp;
    memcpy(out,             rb->buf + rb->rp,   first);
    rb->rp = (rb->rp + first) % rb->capacity;
    if (n > first) {
        memcpy(out + first, rb->buf,          n - first);
        rb->rp = (rb->rp + n - first) % rb->capacity;
    }

    rb->sz -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0 || rb->sz == 0) return 0;
    size_t n = len < rb->sz ? len : rb->sz;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[(rb->rp + i) % rb->capacity];
    }
    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->sz;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->sz = rb->wp = rb->rp = 0;
}

/* ── test ── */
#ifdef RINGBUF_TEST

#include <stdio.h>

static int ok(int cond, const char *msg) {
    if (!cond) { printf("FAIL: %s\n", msg); return 1; }
    return 0;
}

int main(void) {
    int fail = 0;

    ringbuf_t *rb = rb_create(16);
    fail += ok(rb != NULL, "create");
    fail += ok(rb_capacity(rb) == 16, "capacity");
    fail += ok(rb_size(rb) == 0, "initial size");

    /* NULL / empty edge cases */
    fail += ok(rb_write(NULL, (uint8_t *)"", 1) == 0, "write NULL rb");
    fail += ok(rb_read(NULL,  (uint8_t *)"", 1) == 0, "read NULL rb");
    fail += ok(rb_peek(NULL,(uint8_t *)"", 1) == 0,   "peek NULL rb");
    fail += ok(rb_size(NULL) == 0,                     "size NULL rb");
    fail += ok(rb_capacity(NULL) == 0,                "cap NULL rb");

    uint8_t hello[] = {72, 101, 108, 108, 111}; /* "Hello" */
    size_t w = rb_write(rb, hello, sizeof(hello));
    fail += ok(w == 5, "write 5");
    fail += ok(rb_size(rb) == 5, "size after write 5");

    uint8_t three[4];
    size_t r = rb_read(rb, three, 3);
    fail += ok(r == 3, "read 3");
    fail += ok(memcmp(three, hello, 3) == 0, "contents match read");
    fail += ok(rb_size(rb) == 2, "size after read 3");

    /* peek */
    uint8_t p[5];
    size_t pr = rb_peek(rb, p, 10);
    fail += ok(pr == 2, "peek past data");
    fail += ok(p[0] == hello[3] && p[1] == hello[4], "peek content");

    /* fill remaining capacity */
    uint8_t extra[] = {1,2,3,4,5};
    w = rb_write(rb, extra, sizeof(extra));
    fail += ok(w == 7, "write remainder of cap (2 leftover+5)");
    fail += ok(rb_size(rb) == 9, "size after partial fill");

    /* write more: only 7 fit (16-9=7) */
    uint8_t m[10];
    w = rb_write(rb, m, sizeof(m));
    fail += ok(w == 7 && ((ringbuf_t*)rb)->sz == 16, "write clamps at total cap");

    /* drain all */
    uint8_t drained[20];
    r = rb_read(rb, drained, 20);
    fail += ok(r == 16, "read all after fill clamp");
    fail += ok(rb_size(rb) == 0, "after drain");

    /* wrap-around test: write past the capacity boundary */
    uint8_t onebyte = 0xAB;
    for (int i = 0; i < 15; i++) rb_write(rb, &onebyte, 1);
    fail += ok(rb_size(rb) == 15 && ((ringbuf_t*)rb)->wp == 15, "fill to 15");

    /* clear */
    rb_clear(rb);
    fail += ok(rb_size(rb) == 0 && rb_capacity(rb) == 16, "clear preserved cap and zeroed sz");

    /* invalid create */
    fail += ok(rb_create(0) == NULL, "create zero capacity returns NULL");

    /* destroy NULL is fine */
    rb_destroy(NULL);

    /* wrap-around read test: fill half, write 12 (crosses boundary), verify round-trip */
    rb_clear(rb);
    uint8_t fill3[] = {3,4,5};
    w = rb_write(rb, fill3, sizeof(fill3));
    fail += ok(w == 3 && ((ringbuf_t*)rb)->wp == 3, "write partial boundary");

    uint8_t big[20];
    memset(big, 0xCD, sizeof(big));
    w = rb_write(rb, big, sizeof(big));
    size_t cap_remaining = (16 - 3);
    fail += ok(w == cap_remaining, "big write clamps");

    uint8_t roundtripped[20];
    r = rb_read(rb, roundtripped, ((ringbuf_t*)rb)->sz);
    /* first 3 bytes should be fill3 (wrap position) + the rest should be big starting from where wrap happened */
    fail += ok(r == w && roundtripped[0] == fill3[0] && roundtripped[1] == fill3[1] &&
               ((ringbuf_t*)rb)->sz == 0, "roundtrip after wrap");

    rb_destroy(rb);

    printf("DONE: %d failures\n", fail);
    return fail > 0;
}
#endif