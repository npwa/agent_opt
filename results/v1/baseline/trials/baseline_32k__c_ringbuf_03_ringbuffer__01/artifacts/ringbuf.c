#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buffer;
    size_t capacity;
    size_t length;
    size_t head;
    size_t tail;
};

ringbuf_t *rb_create(size_t cap)
{
    if (cap == 0)
        return NULL;

    ringbuf_t *rb = calloc(1, sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buffer = malloc(cap);
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }

    rb->capacity = cap;
    rb->length   = 0;
    rb->head     = 0;
    rb->tail     = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buffer);
    free(rb);
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->length;
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
    rb->length = rb->head = rb->tail = 0;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;
    if (rb->length >= rb->capacity)
        return 0;

    if (len > rb->capacity - rb->length)
        len = rb->capacity - rb->length;

    size_t written = 0;
    while (written < len) {
        rb->buffer[rb->head] = data[written];
        rb->head++;
        if (rb->head == rb->capacity)
            rb->head = 0;
        written++;
    }

    rb->length += written;
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;
    if (rb->length == 0)
        return 0;

    if (len > rb->length)
        len = rb->length;

    size_t read_count = 0;
    while (read_count < len) {
        out[read_count] = rb->buffer[rb->tail];
        rb->tail++;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
        read_count++;
    }

    rb->length -= read_count;
    return read_count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;
    if (rb->length == 0)
        return 0;

    if (len > rb->length)
        len = rb->length;

    size_t pos = rb->tail;
    for (size_t i = 0; i < len; i++) {
        out[i] = rb->buffer[pos];
        pos++;
        if (pos == rb->capacity)
            pos = 0;
    }

    return len;
}
