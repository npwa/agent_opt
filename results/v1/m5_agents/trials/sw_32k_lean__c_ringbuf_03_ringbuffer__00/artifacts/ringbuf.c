#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *data;
    size_t capacity;
    size_t head;         /* index of next byte to read (oldest) */
    size_t tail;         /* index of next free slot (newest+1) */
    size_t count;        /* bytes currently stored in the buffer */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->data = malloc(capacity);
    if (!rb->data) {
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
    free(rb->data);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t n, i;

    if (!rb || !data || len == 0)
        return 0;

    n = len;
    if (n > rb->capacity - rb->count)
        n = rb->capacity - rb->count;

    for (i = 0; i < n; i++) {
        rb->data[rb->tail] = data[i];
        rb->tail++;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
    }
    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n, i;

    if (!rb || !out || len == 0)
        return 0;

    n = len;
    if (n > rb->count)
        n = rb->count;

    for (i = 0; i < n; i++) {
        out[i] = rb->data[rb->head];
        rb->head++;
        if (rb->head == rb->capacity)
            rb->head = 0;
    }
    rb->count -= n;

    /* Keep head/tail compact when empty */
    if (rb->count == 0) {
        rb->head   = 0;
        rb->tail   = 0;
    }
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    /* peek does not mutate anything */
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    n = len;
    if (n > rb->count)
        n = rb->count;

    for (size_t i = 0; i < n; i++) {
        out[i] = rb->data[(rb->head + i) % rb->capacity];
    }
    return n;
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
    rb->count = 0;
    rb->head  = 0;
    rb->tail  = 0;
}
