#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

/*
 * Ring buffer layout:
 *   head = index of oldest byte (read pointer)
 *   tail = index where next byte will be written (write pointer)
 *   size = number of valid bytes currently in the buffer
 *
 * The total physical region is [0, capacity-1].
 * Valid data occupies [head .. tail) wrapping modulo capacity.
 * When head == tail and size > 0 the buffer is full; that's distinguishable by size.
 */

struct ringbuf {
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
    uint8_t buf[];
};

static int null_arg(const void *p) { return !p; }

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    struct ringbuf *rb = calloc(1, sizeof(*rb) + capacity);
    if (!rb) return NULL;
    rb->capacity = capacity;
    rb->size     = 0;
    rb->head     = 0;
    rb->tail     = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (null_arg(rb) || null_arg(data) || len == 0) return 0;
    /* capacity is always > 0 here */
    size_t free = rb->capacity - rb->size;
    size_t n    = len < free ? len : free;
    for (size_t i = 0; i < n; ++i)
        rb->buf[(rb->tail + i) % rb->capacity] = data[i];
    rb->tail     = (rb->tail + n) % rb->capacity;
    rb->size     += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (null_arg(rb) || null_arg(out)) return 0;
    if (!rb || !out || rb->size == 0) return 0;
    size_t n = len < rb->size ? len : rb->size;
    for (size_t i = 0; i < n; ++i)
        out[i] = rb->buf[(rb->head + i) % rb->capacity];
    rb->head     = (rb->head + n) % rb->capacity;
    rb->size     -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (null_arg(rb) || null_arg(out)) return 0;
    if (!rb || !out || rb->size == 0) return 0;
    size_t n = len < rb->size ? len : rb->size;
    for (size_t i = 0; i < n; ++i)
        out[i] = rb->buf[(rb->head + i) % rb->capacity];
    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->size;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->size = 0;
    rb->head = 0;
}
