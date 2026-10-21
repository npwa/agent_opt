#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* write index */
    size_t tail;   /* read index */
    size_t count;  /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0)
        return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;
    rb->buf = calloc(capacity, sizeof(uint8_t));
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

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

static size_t ring_capacity(const ringbuf_t *rb) {
    return rb->capacity - 1;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0)
        return 0;
    size_t cap = ring_capacity(rb);
    if (rb->count >= cap)
        return 0;
    size_t free_slots = cap - rb->count;
    size_t n = len < free_slots ? len : free_slots;
    /* Write may wrap around */
    size_t first = rb->capacity - rb->head; /* bytes from head to end */
    if (n <= first) {
        memcpy(rb->buf + rb->head, data, n);
    } else {
        memcpy(rb->buf + rb->head, data, first);
        memcpy(rb->buf, data + first, n - first);
    }
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
    size_t first = rb->capacity - rb->tail;
    if (n <= first) {
        memcpy(out, rb->buf + rb->tail, n);
    } else {
        memcpy(out, rb->buf + rb->tail, first);
        memcpy(out + first, rb->buf, n - first);
    }
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
    size_t first = rb->capacity - rb->tail;
    if (n <= first) {
        memcpy(out, rb->buf + rb->tail, n);
    } else {
        memcpy(out, rb->buf + rb->tail, first);
        memcpy(out + first, rb->buf, n - first);
    }
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
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
