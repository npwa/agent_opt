#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *store;
    size_t capacity;
    size_t size;  /* bytes in buffer */
    size_t put_pos;
    size_t get_pos;
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->store = calloc(capacity, sizeof(uint8_t));
    if (!rb->store) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->put_pos = 0;
    rb->get_pos = 0;
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

    size_t free_space = rb->capacity - rb->size;
    if (free_space == 0)
        return 0;

    size_t to_write = len < free_space ? len : free_space;

    for (size_t i = 0; i < to_write; i++) {
        rb->store[rb->put_pos] = data[i];
        rb->put_pos = (rb->put_pos + 1) % rb->capacity;
    }

    rb->size += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->size == 0)
        return 0;

    size_t to_read = len < rb->size ? len : rb->size;

    for (size_t i = 0; i < to_read; i++) {
        out[i] = rb->store[rb->get_pos];
        rb->get_pos = (rb->get_pos + 1) % rb->capacity;
    }

    rb->size -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->size == 0)
        return 0;

    size_t to_peek = len < rb->size ? len : rb->size;

    size_t pos = rb->get_pos;
    for (size_t i = 0; i < to_peek; i++) {
        out[i] = rb->store[pos];
        pos = (pos + 1) % rb->capacity;
    }

    return to_peek;
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
    rb->put_pos = 0;
    rb->get_pos = 0;
}
