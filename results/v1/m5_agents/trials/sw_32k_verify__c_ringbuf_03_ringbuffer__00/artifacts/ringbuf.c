#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;  /* next write position */
    size_t tail;  /* next read position   */
    size_t count; /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->storage = calloc(capacity, sizeof(uint8_t));
    if (!rb->storage) {
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
    free(rb->storage);
    free(rb);
}

static size_t min_sz(size_t a, size_t b)
{
    return a < b ? a : b;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t free_space = rb->capacity - rb->count;
    size_t to_write   = min_sz(len, free_space);

    if (to_write == 0)
        return 0;

    /* First chunk: from head up to end of storage */
    size_t first = min_sz(to_write, rb->capacity - rb->head);
    memcpy(rb->storage + rb->head, data, first);

    /* Second chunk (possibly zero): wraps around to start */
    size_t second = to_write - first;
    if (second > 0)
        memcpy(rb->storage, data + first, second);

    rb->head += to_write;
    if (rb->head >= rb->capacity)
        rb->head -= rb->capacity;
    rb->count += to_write;

    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->count;
    size_t to_read = min_sz(len, avail);

    if (to_read == 0)
        return 0;

    /* First chunk: from tail up to end of storage */
    size_t first = min_sz(to_read, rb->capacity - rb->tail);
    memcpy(out, rb->storage + rb->tail, first);

    /* Second chunk (possibly zero): wraps around to start */
    size_t second = to_read - first;
    if (second > 0)
        memcpy(out + first, rb->storage, second);

    rb->tail += to_read;
    if (rb->tail >= rb->capacity)
        rb->tail -= rb->capacity;
    rb->count -= to_read;

    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->count;
    size_t to_read = min_sz(len, avail);

    if (to_read == 0)
        return 0;

    /* Copy without modifying tail/count — walk manually */
    for (size_t i = 0; i < to_read; ++i) {
        out[i] = rb->storage[(rb->tail + i) % rb->capacity];
    }

    return to_read;
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
