#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t sz;
    size_t head;
    size_t tail;
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
    rb->sz = 0;
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
    size_t avail, first;

    if (!rb || !data || len == 0)
        return 0;

    avail = rb->capacity - rb->sz;
    if (avail == 0)
        return 0;

    if (len > avail)
        len = avail;

    first = rb->capacity - rb->tail;

    if (len <= first) {
        memcpy(rb->buf + rb->tail, data, len);
    } else {
        memcpy(rb->buf + rb->tail, data, first);
        memcpy(rb->buf, data + first, len - first);
    }

    rb->tail += len;
    if (rb->tail >= rb->capacity)
        rb->tail -= rb->capacity;

    rb->sz += len;
    return len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t first;

    if (!rb || !out || len == 0)
        return 0;

    if (rb->sz == 0)
        return 0;

    if (len > rb->sz)
        len = rb->sz;

    first = rb->capacity - rb->head;

    if (len <= first) {
        memcpy(out, rb->buf + rb->head, len);
    } else {
        memcpy(out, rb->buf + rb->head, first);
        memcpy(out + first, rb->buf, len - first);
    }

    rb->head += len;
    if (rb->head >= rb->capacity)
        rb->head -= rb->capacity;

    rb->sz -= len;
    return len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t first;

    if (!rb || !out || len == 0 || rb->sz == 0)
        return 0;

    if (len > rb->sz)
        len = rb->sz;

    first = rb->capacity - rb->head;

    if (len <= first) {
        memcpy(out, rb->buf + rb->head, len);
    } else {
        memcpy(out, rb->buf + rb->head, first);
        memcpy(out + first, rb->buf, len - first);
    }

    return len;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->sz;
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
    rb->sz = 0;
    rb->head = 0;
    rb->tail = 0;
}
