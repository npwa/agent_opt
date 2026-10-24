#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head; /* where next write goes */
    size_t tail; /* where next read comes from */
    size_t count; /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;
    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
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
    rb->count = 0;
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
    if (rb->count == rb->capacity)
        return 0;
    size_t free_space = rb->capacity - rb->count;
    size_t to_write = len < free_space ? len : free_space;
    for (size_t i = 0; i < to_write; i++) {
        rb->buf[rb->head] = data[i];
        rb->head++;
        if (rb->head == rb->capacity)
            rb->head = 0;
    }
    rb->count += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;
    if (rb->count == 0)
        return 0;
    size_t to_read = len < rb->count ? len : rb->count;
    for (size_t i = 0; i < to_read; i++) {
        out[i] = rb->buf[rb->tail];
        rb->tail++;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
    }
    rb->count -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;
    if (rb->count == 0)
        return 0;
    size_t to_read = len < rb->count ? len : rb->count;
    size_t tail = rb->tail;
    for (size_t i = 0; i < to_read; i++) {
        out[i] = rb->buf[tail];
        tail++;
        if (tail == rb->capacity)
            tail = 0;
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
