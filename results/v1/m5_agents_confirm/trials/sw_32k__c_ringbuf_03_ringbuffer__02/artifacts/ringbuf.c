#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head; /* write index */
    size_t tail; /* read index */
    size_t count;
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0)
        return NULL;
    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;
    rb->buf = malloc(capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

static void rb_init(ringbuf_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0)
        return 0;
    size_t free_space = rb->capacity - rb->count;
    size_t n = len < free_space ? len : free_space;

    /* Write to fill from head wrapping around */
    size_t first_chunk = n;
    if (first_chunk > rb->capacity - rb->head)
        first_chunk = rb->capacity - rb->head;

    memcpy(rb->buf + rb->head, data, first_chunk);
    if (n > first_chunk)
        memcpy(rb->buf, data + first_chunk, n - first_chunk);

    rb->head = (rb->head + n) % rb->capacity;
    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;
    if (rb->count == 0)
        return 0;
    size_t n = len < rb->count ? len : rb->count;

    size_t first_chunk = n;
    if (first_chunk > rb->capacity - rb->tail)
        first_chunk = rb->capacity - rb->tail;

    memcpy(out, rb->buf + rb->tail, first_chunk);
    if (n > first_chunk)
        memcpy(out + first_chunk, rb->buf, n - first_chunk);

    rb->tail = (rb->tail + n) % rb->capacity;
    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;
    if (rb->count == 0)
        return 0;
    size_t n = len < rb->count ? len : rb->count;
    size_t tmp_tail = rb->tail;

    /* Copy without mutating the buffer */
    size_t first_chunk = n;
    if (first_chunk > rb->capacity - tmp_tail)
        first_chunk = rb->capacity - tmp_tail;

    memcpy(out, rb->buf + tmp_tail, first_chunk);
    if (n > first_chunk)
        memcpy(out + first_chunk, rb->buf, n - first_chunk);

    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb)
        return;
    rb_init(rb);
}
