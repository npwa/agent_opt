#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *store;
    size_t capacity;
    size_t head;  /* write position (monotonically increases) */
    size_t tail;  /* read position  (monotonically increases) */
    size_t count; /* bytes currently stored                       */
};

/* --- public API --- */

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->store = calloc(capacity, sizeof(uint8_t));
    if (!rb->store) {
        free(rb);
        return NULL;
    }

    rb->capacity   = capacity;
    rb->head       = 0;
    rb->tail       = 0;
    rb->count      = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->store);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = rb->capacity - rb->count;
    size_t wrote = len < avail ? len : avail;

    for (size_t i = 0; i < wrote; ++i) {
        rb->store[(rb->head++) % rb->capacity] = data[i];
    }

    rb->count += wrote;
    return wrote;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t rd = len < rb->count ? len : rb->count;

    for (size_t i = 0; i < rd; ++i) {
        out[i] = rb->store[(rb->tail++) % rb->capacity];
    }

    rb->count -= rd;
    return rd;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t n = len < rb->count ? len : rb->count;

    for (size_t i = 0; i < n; ++i) {
        out[i] = rb->store[(rb->tail + i) % rb->capacity];
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
    rb->head   = 0;
    rb->tail   = 0;
    rb->count  = 0;
}
