#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* write position */
    size_t tail;   /* read position */
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
    size_t nwritten;
    size_t space;

    if (!rb || !data || len == 0)
        return 0;

    space = rb->capacity - rb->count;
    if (len > space)
        len = space;

    nwritten = len;

    /* First segment: from head to end of buffer */
    {
        size_t first = rb->capacity - rb->head;
        if (len >= first) {
            memcpy(rb->buf + rb->head, data, first);
            rb->head = 0;
            len -= first;
            data += first;
        } else {
            memcpy(rb->buf + rb->head, data, len);
            rb->head += len;
            rb->count += nwritten;
            return nwritten;
        }
    }

    /* Second segment: wraps around to start */
    if (len > 0) {
        memcpy(rb->buf + rb->head, data, len);
        rb->head = len;
    }

    rb->count += nwritten;
    return nwritten;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t nread;
    size_t avail;

    if (!rb || !out || len == 0)
        return 0;

    avail = rb->count;
    if (len > avail)
        len = avail;

    nread = len;

    /* First segment: from tail to end of buffer */
    {
        size_t first = rb->capacity - rb->tail;
        if (len >= first) {
            memcpy(out, rb->buf + rb->tail, first);
            out += first;
            rb->tail = 0;
            len -= first;
        } else {
            memcpy(out, rb->buf + rb->tail, len);
            rb->tail += len;
            rb->count -= nread;
            return nread;
        }
    }

    /* Second segment: wraps around to start */
    if (len > 0) {
        memcpy(out, rb->buf + rb->tail, len);
        rb->tail = len;
    }

    rb->count -= nread;
    return nread;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t avail;
    size_t orig_len;

    if (!rb || !out || len == 0)
        return 0;

    avail = rb->count;
    orig_len = len;
    if (len > avail)
        len = avail;

    /* First segment: from tail to end of buffer */
    {
        size_t first = rb->capacity - rb->tail;
        if (len >= first) {
            memcpy(out, rb->buf + rb->tail, first);
            out += first;
            len -= first;
        } else {
            memcpy(out, rb->buf + rb->tail, len);
            return orig_len;
        }
    }

    /* Second segment: wraps around to start */
    if (len > 0) {
        memcpy(out, rb->buf + rb->tail, len);
    }

    return orig_len;
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
