#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;  /* write position */
    size_t tail;  /* read position */
    size_t count; /* number of bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->storage = calloc(capacity, sizeof(uint8_t));
    if (!rb->storage) {
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
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t n;

    if (!rb || !data || len == 0)
        return 0;

    /* If full */
    if (rb->count == rb->capacity)
        return 0;

    n = len;
    if (n > rb->capacity - rb->count)
        n = rb->capacity - rb->count;

    /* Write in possibly two segments due to wrap-around */
    const size_t tail_end = rb->capacity - rb->tail;
    const size_t before_wrap = (n > tail_end) ? tail_end : n;

    memcpy(rb->storage + rb->head, data, before_wrap);
    if (n > before_wrap) {
        memcpy(rb->storage, data + before_wrap, n - before_wrap);
    }

    rb->head = (rb->head + n) % rb->capacity;
    rb->count += n;

    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    n = len;
    if (n > rb->count)
        n = rb->count;

    /* Read in possibly two segments due to wrap-around */
    const size_t end_at_wrap = rb->capacity - rb->tail;
    const size_t before_wrap = (n > end_at_wrap) ? end_at_wrap : n;

    memcpy(out, rb->storage + rb->tail, before_wrap);
    if (n > before_wrap) {
        memcpy(out + before_wrap, rb->storage, n - before_wrap);
    }

    rb->tail = (rb->tail + n) % rb->capacity;
    rb->count -= n;

    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    const size_t n = len < rb->count ? len : rb->count;

    /* Peek in possibly two segments due to wrap-around */
    const size_t end_at_wrap = rb->capacity - rb->tail;
    const size_t before_wrap = (n > end_at_wrap) ? end_at_wrap : n;

    memcpy(out, rb->storage + rb->tail, before_wrap);
    if (n > before_wrap) {
        memcpy(out + before_wrap, rb->storage, n - before_wrap);
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
