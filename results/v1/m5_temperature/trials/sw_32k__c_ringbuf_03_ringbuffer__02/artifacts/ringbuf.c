#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->storage = malloc(capacity);
    if (!rb->storage) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size     = 0;
    rb->head     = 0;
    rb->tail     = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0)
        return 0;

    size_t n = 0;
    while (n < len && rb->size < rb->capacity) {
        rb->storage[rb->head] = data[n];
        rb->head = (rb->head + 1) % rb->capacity;
        ++rb->size;
        ++n;
    }

    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;

    size_t n = 0;
    while (n < len && rb->size > 0) {
        out[n] = rb->storage[rb->tail];
        rb->tail = (rb->tail + 1) % rb->capacity;
        --rb->size;
        ++n;
    }

    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t n = 0;
    size_t to_copy = len < rb->size ? len : rb->size;
    size_t tmp     = rb->tail;

    while (n < to_copy) {
        out[n] = rb->storage[tmp];
        tmp    = (tmp + 1) % rb->capacity;
        ++n;
    }

    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->size;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb)
        return;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
}
