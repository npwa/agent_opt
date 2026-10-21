#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;  /* write position (next byte to write) */
    size_t tail;  /* read position (oldest byte) */
    size_t sz;    /* current number of filled slots */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

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
    rb->sz = 0;
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
    size_t to_write;

    if (!rb || !data || len == 0)
        return 0;

    /* Free space = capacity - sz */
    to_write = len;
    if (to_write > rb->capacity - rb->sz)
        to_write = rb->capacity - rb->sz;

    for (size_t i = 0; i < to_write; ++i) {
        rb->buf[(rb->head + i) % rb->capacity] = data[i];
    }
    rb->head = (rb->head + to_write) % rb->capacity;
    rb->sz += to_write;

    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_read;

    if (!rb || !out || len == 0)
        return 0;

    to_read = len;
    if (to_read > rb->sz)
        to_read = rb->sz;

    for (size_t i = 0; i < to_read; ++i) {
        out[i] = rb->buf[(rb->tail + i) % rb->capacity];
    }
    rb->tail = (rb->tail + to_read) % rb->capacity;
    rb->sz -= to_read;

    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_copy;

    if (!rb || !out || len == 0)
        return 0;

    to_copy = len;
    if (to_copy > rb->sz)
        to_copy = rb->sz;

    for (size_t i = 0; i < to_copy; ++i) {
        out[i] = rb->buf[(rb->tail + i) % rb->capacity];
    }

    return to_copy;
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
    rb->head = 0;
    rb->tail = 0;
    rb->sz = 0;
}
