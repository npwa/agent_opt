#include "ringbuf.h"
#include <stdlib.h>

/* Internal layout — opaque to callers because the header only declares
   ringbuf_t as a forward-declared typedef. */
struct ringbuf {
    uint8_t *buf; /* allocated storage: (cap + 1) bytes so head and tail
                       indices can differ by up to cap without extra math */
    size_t head;  /* next write index into buf */
    size_t tail;  /* next read index into buf */
    size_t size;  /* current number of bytes stored */
    size_t cap;   /* capacity passed at creation time */
};

/* ---------- helpers ---------- */

static inline size_t _advance(const ringbuf_t *rb, size_t idx)
{
    return (idx + 1) % (rb->cap + 1);
}

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0) /* "capacity > 0" in the header contract */
        return NULL;

    ringbuf_t *rb = calloc(sizeof(*rb), 1);
    if (!rb)
        return NULL;

    rb->buf = calloc(capacity + 1, 1);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    rb->cap = capacity;

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

    size_t free = rb->cap - rb->size;
    size_t wrote = len <= free ? len : free; /* writes fit as much as it can */

    for (size_t i = 0; i < wrote; ++i) {
        rb->buf[rb->head] = data[i];
        rb->head = _advance(rb, rb->head);
    }
    rb->size += wrote;
    return wrote;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t rd = len < rb->size ? len : rb->size;

    for (size_t i = 0; i < rd; ++i) {
        out[i] = rb->buf[rb->tail];
        rb->tail = _advance(rb, rb->tail);
    }
    rb->size -= rd;
    return rd;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t rd = len < rb->size ? len : rb->size;

    size_t idx = rb->tail;
    for (size_t i = 0; i < rd; ++i) {
        out[i] = rb->buf[idx];
        idx = _advance(rb, idx);
    }
    return rd;
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
    return rb->cap;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
