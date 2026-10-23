#include "ringbuf.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* next write position */
    size_t tail;   /* next read position  */
    size_t count;  /* bytes currently stored */
};

static size_t min_size(size_t a, size_t b) { return a < b ? a : b; }

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(*rb));
    if (!rb)
        return NULL;
    rb->buf = (uint8_t *)calloc(capacity, sizeof(uint8_t));
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

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data)
        return 0;
    if (len == 0 || rb->count == rb->capacity)
        return 0;

    size_t avail = rb->capacity - rb->count;
    size_t to_write = min_size(len, avail);

    /* Case A: the write fits in a single contiguous span */
    if (rb->head + to_write <= rb->capacity) {
        memcpy(rb->buf + rb->head, data, to_write);
    } else {
        /* Case B: must wrap around */
        size_t first_part = rb->capacity - rb->head; /* bytes until end of buffer */
        memcpy(rb->buf + rb->head, data, first_part);
        memcpy(rb->buf, data + first_part, to_write - first_part);
    }

    rb->head = (rb->head + to_write) % rb->capacity;
    rb->count += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0)
        return 0;

    size_t to_read = min_size(len, rb->count);

    if (rb->tail + to_read <= rb->capacity) {
        memcpy(out, rb->buf + rb->tail, to_read);
    } else {
        size_t first_part = rb->capacity - rb->tail;
        memcpy(out, rb->buf + rb->tail, first_part);
        memcpy(out + first_part, rb->buf, to_read - first_part);
    }

    rb->tail = (rb->tail + to_read) % rb->capacity;
    rb->count -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0)
        return 0;

    size_t to_copy = min_size(len, rb->count);

    if (rb->tail + to_copy <= rb->capacity) {
        memcpy(out, rb->buf + rb->tail, to_copy);
    } else {
        size_t first_part = rb->capacity - rb->tail;
        memcpy(out, rb->buf + rb->tail, first_part);
        memcpy(out + first_part, rb->buf, to_copy - first_part);
    }

    return to_copy;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->count;
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
    rb->count = 0;
}
