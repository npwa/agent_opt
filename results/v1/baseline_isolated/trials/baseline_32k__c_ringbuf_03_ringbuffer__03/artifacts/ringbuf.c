#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t sz;
    size_t rd; /* read index */
    size_t wr; /* write index */
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->buf = malloc(capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->sz = 0;
    rb->rd = 0;
    rb->wr = 0;

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

    /* how many bytes fit */
    size_t avail = rb->capacity - rb->sz;
    size_t n = MIN(len, avail);

    for (size_t i = 0; i < n; i++) {
        rb->buf[rb->wr] = data[i];
        rb->wr = (rb->wr + 1) % rb->capacity;
        rb->sz++;
    }

    return n;
}

/* Helper: advance read index through the buffer, returning number of bytes consumed */
static size_t consume(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t n = MIN(len, rb->sz);

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[rb->rd];
        rb->rd = (rb->rd + 1) % rb->capacity;
    }
    rb->sz -= n;

    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    return consume(rb, out, len);
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t n = MIN(len, rb->sz);

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[(rb->rd + i) % rb->capacity];
    }

    return n;
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
    rb->rd = 0;
    rb->wr = 0;
    rb->sz = 0;
}
