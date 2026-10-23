#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ringbuf.h"

struct ringbuf {
    uint8_t *buf;
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    rb->buf = aligned_alloc(sizeof(size_t), capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head = rb->tail = rb->size = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (rb) {
        free(rb->buf);
        free(rb);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    size_t free = rb->capacity - rb->size;
    size_t n = len > free ? free : len;
    size_t wrap = (rb->head + free) >= rb->capacity;
    
    if (wrap) {
        memcpy(rb->buf + rb->head, data, rb->capacity - rb->head);
        memcpy(rb->buf, data + (rb->capacity - rb->head), n - (rb->capacity - rb->head));
        rb->head = rb->capacity - (n - (rb->capacity - rb->head));
    } else {
        memcpy(rb->buf + rb->head, data, n);
        rb->head += n;
    }
    rb->size += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0 || rb->size == 0) return 0;
    size_t n = len > rb->size ? rb->size : len;
    size_t wrap = (rb->tail + n) >= rb->capacity;
    
    if (wrap) {
        memcpy(out, rb->buf + rb->tail, rb->capacity - rb->tail);
        memcpy(out + (rb->capacity - rb->tail), rb->buf, n - (rb->capacity - rb->tail));
        rb->tail = rb->capacity - (n - (rb->capacity - rb->tail));
    } else {
        memcpy(out, rb->buf + rb->tail, n);
        rb->tail += n;
    }
    rb->size -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0 || rb->size == 0) return 0;
    size_t n = len > rb->size ? rb->size : len;
    size_t wrap = (rb->tail + n) >= rb->capacity;
    
    if (wrap) {
        memcpy(out, rb->buf + rb->tail, rb->capacity - rb->tail);
        memcpy(out + (rb->capacity - rb->tail), rb->buf, n - (rb->capacity - rb->tail));
    } else {
        memcpy(out, rb->buf + rb->tail, n);
    }
    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    return rb ? rb->size : 0;
}

size_t rb_capacity(const ringbuf_t *rb) {
    return rb ? rb->capacity : 0;
}

void rb_clear(ringbuf_t *rb) {
    if (rb) {
        rb->head = rb->tail = rb->size = 0;
    }
}