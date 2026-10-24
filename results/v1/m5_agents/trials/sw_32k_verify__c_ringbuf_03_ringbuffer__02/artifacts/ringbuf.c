#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *buf;
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
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

    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    rb->capacity = capacity;
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
    if (!rb || !data || rb->size == rb->capacity)
        return 0;

    size_t avail = rb->capacity - rb->size;
    size_t written = len < avail ? len : avail;

    for (size_t i = 0; i < written; i++) {
        size_t pos = (rb->head + i) % rb->capacity;
        rb->buf[pos] = data[i];
    }
    rb->head = (rb->head + written) % rb->capacity;
    rb->size += written;

    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->size == 0)
        return 0;

    size_t rd = len < rb->size ? len : rb->size;

    for (size_t i = 0; i < rd; i++) {
        size_t pos = (rb->tail + i) % rb->capacity;
        out[i] = rb->buf[pos];
    }
    rb->tail = (rb->tail + rd) % rb->capacity;
    rb->size -= rd;

    return rd;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->size == 0)
        return 0;

    size_t rd = len < rb->size ? len : rb->size;

    for (size_t i = 0; i < rd; i++) {
        size_t pos = (rb->tail + i) % rb->capacity;
        out[i] = rb->buf[pos];
    }

    return rd;
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
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
