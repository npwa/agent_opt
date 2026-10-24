#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->storage = malloc(capacity);
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
    size_t to_end = rb->capacity - rb->head;
    size_t copy_first = n < to_end ? n : to_end;

    memcpy(rb->storage + rb->head, data, copy_first);
    rb->head += copy_first;
    if (rb->head == rb->capacity)
        rb->head = 0;

    if (n > copy_first) {
        size_t remaining = n - copy_first;
        memcpy(rb->storage, data + copy_first, remaining);
        rb->head += remaining;
    }

    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t sz = rb->count;
    if (sz == 0)
        return 0;

    size_t n = len < sz ? len : sz;
    size_t to_end = rb->capacity - rb->tail;
    size_t copy_first = n < to_end ? n : to_end;

    memcpy(out, rb->storage + rb->tail, copy_first);
    rb->tail += copy_first;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    if (n > copy_first) {
        size_t remaining = n - copy_first;
        memcpy(out + copy_first, rb->storage, remaining);
        rb->tail += remaining;
    }

    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t sz = rb->count;
    if (sz == 0)
        return 0;

    size_t n = len < sz ? len : sz;
    size_t to_end = rb->capacity - rb->tail;
    size_t copy_first = n < to_end ? n : to_end;

    memcpy(out, rb->storage + rb->tail, copy_first);

    if (n > copy_first) {
        size_t remaining = n - copy_first;
        memcpy(out + copy_first, rb->storage, remaining);
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
