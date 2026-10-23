#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* index of next write position */
    size_t tail;   /* index of oldest byte (read position) */
    size_t count;  /* bytes currently stored */
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

static size_t free_space(const ringbuf_t *rb)
{
    return rb->capacity - rb->count;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t avail;
    size_t written;

    if (!rb || !data || len == 0)
        return 0;

    avail = free_space(rb);
    if (len > avail)
        len = avail;

    written = len;

    /* First segment: from head to end of buffer */
    {
        size_t first = rb->capacity - rb->head;
        size_t copy = len < first ? len : first;
        memcpy(rb->buf + rb->head, data, copy);
        rb->head += copy;
        if (rb->head == rb->capacity)
            rb->head = 0;
        data += copy;
        len -= copy;
    }

    /* Second segment: wraps around */
    if (len > 0) {
        memcpy(rb->buf, data, len);
        rb->head += len;
    }

    rb->count += written;
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t avail;
    size_t read_count;

    if (!rb || !out || len == 0)
        return 0;

    avail = rb_size(rb);
    if (len > avail)
        len = avail;

    read_count = len;

    /* First segment: from tail to end of buffer */
    {
        size_t first = rb->capacity - rb->tail;
        size_t copy = len < first ? len : first;
        memcpy(out, rb->buf + rb->tail, copy);
        out += copy;
        rb->tail += copy;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
        len -= copy;
    }

    /* Second segment: wraps around */
    if (len > 0) {
        memcpy(out, rb->buf, len);
        rb->tail += len;
    }

    rb->count -= read_count;
    return read_count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t avail;
    size_t peeked;

    if (!rb || !out || len == 0)
        return 0;

    avail = rb_size(rb);
    if (len > avail)
        len = avail;

    peeked = len;

    /* Copy from tail without advancing head */
    {
        size_t first = rb->capacity - rb->tail;
        size_t copy = len < first ? len : first;
        memcpy(out, rb->buf + rb->tail, copy);
        out += copy;
        len -= copy;
    }

    if (len > 0) {
        memcpy(out, rb->buf, len);
    }

    return peeked;
}
