#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t head;  /* next read index */
    size_t sz;
    size_t cap;
};

static int _valid(const ringbuf_t *rb) {
    return rb != NULL && rb->cap > 0 && rb->sz <= rb->cap;
}

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if (!rb) return NULL;
    rb->buf = (uint8_t *)calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) { free(rb); return NULL; }
    rb->head = 0;
    rb->sz = 0;
    rb->cap = capacity;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (rb == NULL) return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!_valid(rb) || data == NULL || len == 0) return 0;
    size_t fw = rb->cap - rb->sz;
    size_t n = len < fw ? len : fw;

    /* where to write */
    size_t wstart = (rb->head + rb->sz) % rb->cap;
    if (n <= rb->cap - wstart) {
        memcpy(rb->buf + wstart, data, n);
    } else {
        size_t first = rb->cap - wstart;
        memcpy(rb->buf + wstart, data, first);
        memcpy(rb->buf, data + first, n - first);
    }

    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!_valid(rb) || out == NULL || len == 0) return 0;
    if (rb->sz == 0) return 0;
    size_t n = len < rb->sz ? len : rb->sz;

    memcpy(out, rb->buf + rb->head % rb->cap, n);
    rb->sz -= n;
    rb->head = (rb->head + n) % rb->cap;

    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!_valid(rb) || out == NULL || len == 0) return 0;
    if (rb->sz == 0) return 0;
    size_t n = len < rb->sz ? len : rb->sz;

    memcpy(out, rb->buf + rb->head % rb->cap, n);
    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!_valid(rb)) return 0;
    return rb->sz;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->cap;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->sz = 0;
}
