#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;    /* write index */
    size_t tail;    /* read index */
    size_t count;   /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

    if (capacity == 0)
        return NULL;

    rb = (ringbuf_t *)calloc(1, sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->buf = (uint8_t *)malloc(capacity);
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
    size_t n;

    if (!rb || !data || len == 0)
        return 0;

    /* space available = capacity - count */
    n = len < (rb->capacity - rb->count) ? len : (rb->capacity - rb->count);

    for (size_t i = 0; i < n; i++) {
        rb->buf[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->capacity;
    }

    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    n = len < rb->count ? len : rb->count;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[rb->tail];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;
    size_t idx;

    if (!rb || !out || len == 0)
        return 0;

    n = len < rb->count ? len : rb->count;
    idx = rb->tail;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->buf[idx];
        idx = (idx + 1) % rb->capacity;
    }

    return n;
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
