#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t count;
    size_t head;  /* oldest byte */
    size_t tail;  /* next write position */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;
    if (capacity == 0)
        return NULL;
    rb = calloc(1, sizeof(*rb));
    if (!rb)
        return NULL;
    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
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
    size_t free_space;
    size_t to_write;
    size_t i;

    if (rb == NULL || data == NULL || len == 0)
        return 0;

    free_space = rb->capacity - rb->count;
    to_write = len < free_space ? len : free_space;
    if (to_write == 0)
        return 0;

    for (i = 0; i < to_write; i++) {
        rb->buf[(rb->tail + i) % rb->capacity] = data[i];
    }
    rb->tail = (rb->tail + to_write) % rb->capacity;
    rb->count += to_write;

    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_read;

    if (rb == NULL || out == NULL || len == 0 || rb->count == 0)
        return 0;

    to_read = len < rb->count ? len : rb->count;

    {
        size_t first = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head, first < to_read ? first : to_read);
        out += (first < to_read ? first : to_read);

        if ((rb->head + to_read) > rb->capacity) {
            size_t remaining = to_read - first;
            memcpy(out, rb->buf, remaining);
        }
    }
    rb->head = (rb->head + to_read) % rb->capacity;
    rb->count -= to_read;

    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t to_read;

    if (rb == NULL || out == NULL || len == 0 || rb->count == 0)
        return 0;

    to_read = len < rb->count ? len : rb->count;

    {
        size_t first = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head, first < to_read ? first : to_read);
        out += (first < to_read ? first : to_read);

        if ((rb->head + to_read) > rb->capacity) {
            size_t remaining = to_read - first;
            memcpy(out, rb->buf, remaining);
        }
    }

    return to_read;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (rb == NULL)
        return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (rb == NULL)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (rb == NULL)
        return;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
}
