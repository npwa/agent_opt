#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

typedef struct ringbuf {
    uint8_t *storage;
    size_t capacity;       /* fixed capacity of storage                */
    size_t head;           /* next write index                         */
    size_t tail;           /* next read index                          */
    size_t count;          /* bytes currently stored (0..capacity)     */
} ringbuf_t;

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    uint8_t *storage = malloc(capacity);
    if (!storage)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) {
        free(storage);
        return NULL;
    }

    rb->storage   = storage;
    rb->capacity  = capacity;
    rb->head      = 0;
    rb->tail      = 0;
    rb->count     = 0;

    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;

    free(rb->storage);
    free(rb);
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

    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    if (len > rb->capacity - rb->count) {
        /* truncate to fit */
        len = rb->capacity - rb->count;
    }

    for (size_t i = 0; i < len; i++)
        rb->storage[(rb->head + i) % rb->capacity] = data[i];

    rb->head = (rb->head + len) % rb->capacity;
    rb->count += len;

    return len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0 || len == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    for (size_t i = 0; i < len; i++)
        out[i] = rb->storage[(rb->tail + i) % rb->capacity];

    rb->tail = (rb->tail + len) % rb->capacity;
    rb->count -= len;

    return len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0 || len == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    for (size_t i = 0; i < len; i++)
        out[i] = rb->storage[(rb->tail + i) % rb->capacity];

    /* count unchanged — this is peek, not read */
    return len;
}
