#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;   /* next write position */
    size_t tail;   /* next read position */
    size_t size;   /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
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
    rb->size = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->storage);
    free(rb);
}

static size_t min_sz(size_t a, size_t b) { return a < b ? a : b; }

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = rb->capacity - rb->size;
    size_t n = min_sz(len, avail);

    /* First chunk: from head to end of storage */
    size_t first = min_sz(n, rb->capacity - rb->head);
    if (first) {
        memcpy(rb->storage + rb->head, data, first);
        rb->head += first;
        if (rb->head == rb->capacity)
            rb->head = 0;
    }

    /* Second chunk: wraps around */
    if (n > first) {
        size_t second = n - first;
        memcpy(rb->storage + rb->head, data + first, second);
        rb->head += second;
    }

    rb->size += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t n = min_sz(len, rb->size);
    if (n == 0)
        return 0;

    /* First chunk: from tail to end of storage */
    size_t first = min_sz(n, rb->capacity - rb->tail);
    if (first) {
        memcpy(out, rb->storage + rb->tail, first);
        rb->tail += first;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
    }

    /* Second chunk: wraps around */
    if (n > first) {
        size_t second = n - first;
        memcpy(out + first, rb->storage + rb->tail, second);
        rb->tail += second;
    }

    rb->size -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = min_sz(len, rb->size);
    if (avail == 0)
        return 0;

    /* Oldest byte is at tail. First chunk extends to end of storage.
     * If data spans past the end, second chunk starts at position 0. */
    size_t first = min_sz(avail, rb->capacity - rb->tail);
    if (first) {
        memcpy(out, rb->storage + rb->tail, first);
        out += first;
    }

    if (avail > first) {
        size_t second = avail - first;
        /* The second chunk always starts at position 0 when it exists */
        memcpy(out, rb->storage, second);
    }

    return avail;
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
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
