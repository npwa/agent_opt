#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    size_t capacity;
    size_t size;
    size_t head; /* write index */
    size_t tail; /* read index */
    uint8_t *buf;
};

ringbuf_t *rb_create(size_t capacity)
{
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
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t space = rb->capacity - rb->size;
    size_t n = len < space ? len : space;

    for (size_t i = 0; i < n; i++) {
        rb->buf[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->capacity;
    }

    rb->size += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    size_t n = len < rb->size ? len : rb->size;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[rb->tail];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    rb->size -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    size_t n = len < rb->size ? len : rb->size;
    size_t idx = rb->tail;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[idx];
        idx = (idx + 1) % rb->capacity;
    }

    return n;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->size;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
}
