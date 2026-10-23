#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t size;
    size_t rpos;
    size_t wpos;
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = malloc(capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->rpos = 0;
    rb->wpos = 0;
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
    size_t count = 0;

    if (!rb || !data || len == 0)
        return 0;

    while (len > 0) {
        size_t room = rb->capacity - rb->size;
        size_t span1, n;

        if (room == 0)
            break;

        /* How many bytes until the end of the buffer? */
        span1 = rb->capacity - rb->wpos;

        /* How many do we actually want to write this pass? */
        n = len < room ? len : room;
        if (n > span1)
            n = span1;

        memcpy(rb->buf + rb->wpos, data, n);
        rb->wpos += n;
        if (rb->wpos == rb->capacity)
            rb->wpos = 0;
        rb->size += n;
        count += n;
        data += n;
        len -= n;
    }

    return count;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t count = 0;

    if (!rb || !out || len == 0)
        return 0;

    while (len > 0 && rb->size > 0) {
        size_t span1, n;

        /* How many bytes until the end of the buffer? */
        span1 = rb->capacity - rb->rpos;

        /* How many do we actually want to read this pass? */
        n = len < rb->size ? len : rb->size;
        if (n > span1)
            n = span1;

        memcpy(out, rb->buf + rb->rpos, n);
        rb->rpos += n;
        if (rb->rpos == rb->capacity)
            rb->rpos = 0;
        rb->size -= n;
        count += n;
        out += n;
        len -= n;
    }

    return count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t count = 0;
    size_t rpos;

    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    rpos = rb->rpos;

    while (len > 0 && count < rb->size) {
        size_t span1, n;

        span1 = rb->capacity - rpos;
        n = len;
        if (count + n > rb->size)
            n = rb->size - count;
        if (n > span1)
            n = span1;

        memcpy(out, rb->buf + rpos, n);
        rpos += n;
        if (rpos == rb->capacity)
            rpos = 0;
        count += n;
        out += n;
        len -= n;
    }

    return count;
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
    rb->rpos = 0;
    rb->wpos = 0;
}
