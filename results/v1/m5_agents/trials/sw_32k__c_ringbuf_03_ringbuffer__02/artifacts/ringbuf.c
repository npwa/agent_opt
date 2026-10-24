#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head; /* write index */
    size_t tail; /* read index */
    size_t size; /* bytes currently stored */
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
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

static void advance(ringbuf_t *rb, size_t *idx)
{
    (*idx)++;
    if (*idx >= rb->capacity)
        *idx = 0;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t free_space = rb->capacity - rb->size;
    size_t count = len > free_space ? free_space : len;

    for (size_t i = 0; i < count; i++) {
        rb->buf[rb->head] = data[i];
        advance(rb, &rb->head);
    }

    rb->size += count;
    return count;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    size_t count = len > rb->size ? rb->size : len;

    for (size_t i = 0; i < count; i++) {
        out[i] = rb->buf[rb->tail];
        advance(rb, &rb->tail);
    }

    rb->size -= count;
    return count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    size_t count = len > rb->size ? rb->size : len;
    size_t idx = rb->tail;

    for (size_t i = 0; i < count; i++) {
        out[i] = rb->buf[idx];
        advance((ringbuf_t *)rb, &idx);
    }

    return count;
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
