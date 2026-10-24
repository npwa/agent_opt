#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *buf;
    size_t   capacity;
    size_t   head;    /* index of next write  */
    size_t   tail;    /* index of next read   */
    size_t   count;   /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = (uint8_t *)malloc(capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
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

    /* How much space is available? */
    const size_t avail = rb->capacity - rb->count;
    if (avail == 0)
        return 0;

    const size_t to_write = (len < avail) ? len : avail;

    for (size_t i = 0; i < to_write; ++i) {
        rb->buf[(rb->head + i) % rb->capacity] = data[i];
    }
    rb->head     = (rb->head + to_write) % rb->capacity;
    rb->count   += to_write;

    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    const size_t have = rb->count;
    if (have == 0)
        return 0;

    const size_t to_read = (len < have) ? len : have;

    for (size_t i = 0; i < to_read; ++i) {
        out[i] = rb->buf[(rb->tail + i) % rb->capacity];
    }
    rb->tail     = (rb->tail + to_read) % rb->capacity;
    rb->count   -= to_read;

    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    const size_t have = rb->count;
    if (have == 0)
        return 0;

    const size_t to_peek = (len < have) ? len : have;

    for (size_t i = 0; i < to_peek; ++i) {
        out[i] = rb->buf[(rb->tail + i) % rb->capacity];
    }
    /* `head`, `tail`, and `count` are NOT modified. */
    return to_peek;
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
    rb->head   = 0;
    rb->tail   = 0;
    rb->count  = 0;
}
