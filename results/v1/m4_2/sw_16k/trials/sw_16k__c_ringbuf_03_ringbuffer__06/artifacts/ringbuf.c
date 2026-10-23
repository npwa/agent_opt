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
    ringbuf_t *rb = NULL;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
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
    *idx = (*idx + 1) % rb->capacity;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t wrote = 0;

    if (!rb || !data || len == 0)
        return 0;

    while (wrote < len && rb->size < rb->capacity) {
        rb->buf[rb->head] = data[wrote];
        advance(rb, &rb->head);
        wrote++;
        rb->size++;
    }
    return wrote;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t rd = 0;

    if (!rb || !out || len == 0)
        return 0;

    while (rd < len && rb->size > 0) {
        out[rd] = rb->buf[rb->tail];
        advance(rb, &rb->tail);
        rd++;
        rb->size--;
    }
    return rd;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t rd = 0;
    size_t idx = 0;

    if (!rb || !out || len == 0)
        return 0;

    idx = rb->tail;
    while (rd < len && rd < rb->size) {
        out[rd] = rb->buf[idx];
        advance((ringbuf_t *)rb, &idx);
        rd++;
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
