#include "ringbuf.h"

#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t size;       /* bytes currently stored */
    size_t head;       /* index of oldest byte (read position) */
    size_t tail;       /* next free write position */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size     = 0;
    rb->head     = 0;
    rb->tail     = 0;

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
    if (!rb || !data || rb->size == rb->capacity)
        return 0;

    size_t avail = rb->capacity - rb->size;
    size_t to_write = (len < avail) ? len : avail;

    for (size_t i = 0; i < to_write; i++) {
        rb->buf[rb->tail] = data[i];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    rb->size += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->size == 0)
        return 0;

    size_t avail = rb->size;
    size_t to_read = (len < avail) ? len : avail;

    for (size_t i = 0; i < to_read; i++) {
        if (out)
            out[i] = rb->buf[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
    }

    rb->size -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->size == 0)
        return 0;

    size_t avail = rb->size;
    size_t to_copy = (len < avail) ? len : avail;

    for (size_t i = 0; i < to_copy; i++) {
        out[i] = rb->buf[(rb->head + i) % rb->capacity];
    }

    return to_copy;
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
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
}
