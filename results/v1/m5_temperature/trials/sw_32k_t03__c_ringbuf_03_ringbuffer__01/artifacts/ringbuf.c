#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;   /* index of oldest byte */
    size_t tail;   /* index where next byte will be written */
    size_t count;  /* number of valid bytes */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
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
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = rb->capacity - rb->count;
    if (avail == 0)
        return 0;

    size_t n = len < avail ? len : avail;
    size_t space_to_end = rb->capacity - rb->tail;
    size_t first_chunk = space_to_end < n ? space_to_end : n;

    memcpy(rb->storage + rb->tail, data, first_chunk);
    rb->tail += first_chunk;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    if (n > first_chunk) {
        memcpy(rb->storage + rb->tail, data + first_chunk, n - first_chunk);
        rb->tail += n - first_chunk;
    }

    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->count;
    if (avail == 0)
        return 0;

    size_t n = len < avail ? len : avail;
    size_t space_to_end = rb->capacity - rb->head;
    size_t first_chunk = space_to_end < n ? space_to_end : n;

    memcpy(out, rb->storage + rb->head, first_chunk);
    rb->head += first_chunk;
    if (rb->head == rb->capacity)
        rb->head = 0;

    if (n > first_chunk) {
        memcpy(out + first_chunk, rb->storage + rb->head, n - first_chunk);
        rb->head += n - first_chunk;
    }

    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->count;
    if (avail == 0)
        return 0;

    size_t n = len < avail ? len : avail;
    size_t space_to_end = rb->capacity - rb->head;
    size_t first_chunk = space_to_end < n ? space_to_end : n;

    memcpy(out, rb->storage + rb->head, first_chunk);
    size_t tmp_head = rb->head + first_chunk;
    if (tmp_head == rb->capacity)
        tmp_head = 0;

    if (n > first_chunk) {
        memcpy(out + first_chunk, rb->storage + tmp_head, n - first_chunk);
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
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
