#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head; /* next write position */
    size_t tail; /* next read position */
    size_t count; /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(ringbuf_t));
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

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t n;

    if (!rb || !data || len == 0)
        return 0;

    if (len > rb->capacity - rb->count)
        len = rb->capacity - rb->count;

    /* First chunk: from head to end of buffer */
    n = rb->capacity - rb->head;
    if (len < n)
        n = len;

    memcpy(rb->buf + rb->head, data, n);
    rb->head += n;
    if (rb->head == rb->capacity)
        rb->head = 0;

    /* Second chunk: wraps around */
    if (len > n) {
        memcpy(rb->buf, data + n, len - n);
        rb->head += (len - n);
    }

    rb->count += len;
    return len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    /* First chunk: from tail to end of buffer */
    n = rb->capacity - rb->tail;
    if (len < n)
        n = len;

    memcpy(out, rb->buf + rb->tail, n);
    rb->tail += n;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    /* Second chunk: wraps around */
    if (len > n) {
        memcpy(out + n, rb->buf, len - n);
        rb->tail += (len - n);
    }

    rb->count -= len;
    return len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;
    size_t tmp_tail;

    if (!rb || !out || len == 0)
        return 0;

    if (len > rb->count)
        len = rb->count;

    /* Copy from a virtual tail starting at rb->tail */
    tmp_tail = rb->tail;

    n = rb->capacity - tmp_tail;
    if (len < n)
        n = len;

    memcpy(out, rb->buf + tmp_tail, n);
    tmp_tail += n;
    if (tmp_tail == rb->capacity)
        tmp_tail = 0;

    if (len > n) {
        memcpy(out + n, rb->buf, len - n);
        tmp_tail += (len - n);
    }

    return len;
}
