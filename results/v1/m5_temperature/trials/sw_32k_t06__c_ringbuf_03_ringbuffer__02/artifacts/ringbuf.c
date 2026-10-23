#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* next write position */
    size_t tail;   /* next read position */
    size_t count;  /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

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

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t written = 0;

    if (!rb || !data || len == 0)
        return 0;

    /* How much free space do we have? */
    if (rb->count == rb->capacity)
        return 0;

    /* Space available = capacity - count */
    size_t avail = rb->capacity - rb->count;
    if (len > avail)
        len = avail;

    written = len;

    /* First segment: from head to end of buffer */
    size_t first = rb->capacity - rb->head;

    if (first >= len) {
        memcpy(rb->buf + rb->head, data, len);
        rb->head = (rb->head + len) % rb->capacity;
    } else {
        memcpy(rb->buf + rb->head, data, first);
        memcpy(rb->buf, data + first, len - first);
        rb->head = (rb->head + len) % rb->capacity;
    }

    rb->count += len;
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t read_count = 0;

    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    read_count = len;

    /* First segment: from tail to end of buffer */
    size_t first = rb->capacity - rb->tail;

    if (first >= len) {
        memcpy(out, rb->buf + rb->tail, len);
        rb->tail = (rb->tail + len) % rb->capacity;
    } else {
        memcpy(out, rb->buf + rb->tail, first);
        memcpy(out + first, rb->buf, len - first);
        rb->tail = (rb->tail + len) % rb->capacity;
    }

    rb->count -= len;
    return read_count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t peeked = 0;

    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    peeked = len;

    /* First segment: from tail to end of buffer */
    size_t first = rb->capacity - rb->tail;

    if (first >= len) {
        memcpy(out, rb->buf + rb->tail, len);
    } else {
        memcpy(out, rb->buf + rb->tail, first);
        memcpy(out + first, rb->buf, len - first);
    }

    return peeked;
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
