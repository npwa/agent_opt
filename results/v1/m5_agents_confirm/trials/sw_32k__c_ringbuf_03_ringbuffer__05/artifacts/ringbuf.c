#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* index of oldest byte */
    size_t tail;   /* index where next write goes */
    size_t filled; /* number of filled slots */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

    if (capacity == 0 || capacity == (size_t)-1 / 256)
        return NULL;

    rb = calloc(1, sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = malloc(capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity  = capacity;
    rb->head      = 0;
    rb->tail      = 0;
    rb->filled    = 0;
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
    size_t to_write;

    if (!rb || !data || len == 0)
        return 0;

    to_write = len;
    if (to_write > rb->capacity - rb->filled)
        to_write = rb->capacity - rb->filled;

    /* Split into two contiguous writes when wrap-around happens */
    if (rb->tail + to_write > rb->capacity) {
        size_t front = rb->capacity - rb->tail;
        memcpy(rb->buf + rb->tail, data, front);
        memcpy(rb->buf, data + front, to_write - front);
    } else {
        memcpy(rb->buf + rb->tail, data, to_write);
    }

    rb->tail = (rb->tail + to_write) % rb->capacity;
    rb->filled += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_read;

    if (!rb || !out || len == 0)
        return 0;

    to_read = len;
    if (to_read > rb->filled)
        to_read = rb->filled;

    if (rb->head + to_read > rb->capacity) {
        size_t front = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head, front);
        memcpy(out + front, rb->buf, to_read - front);
    } else {
        memcpy(out, rb->buf + rb->head, to_read);
    }

    rb->head = (rb->head + to_read) % rb->capacity;
    rb->filled -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_peek;

    if (!rb || !out || len == 0)
        return 0;

    to_peek = len;
    if (to_peek > rb->filled)
        to_peek = rb->filled;

    if (rb->head + to_peek > rb->capacity) {
        size_t front = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head, front);
        memcpy(out + front, rb->buf, to_peek - front);
    } else {
        memcpy(out, rb->buf + rb->head, to_peek);
    }

    return to_peek;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->filled;
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
    rb->head   = 0;
    rb->tail   = 0;
    rb->filled = 0;
}
