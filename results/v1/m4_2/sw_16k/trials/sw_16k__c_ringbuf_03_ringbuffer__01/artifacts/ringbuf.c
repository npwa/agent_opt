#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head; /* write position */
    size_t tail; /* read position */
    size_t sz;   /* current bytes stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
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
    rb->sz = 0;
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
    size_t n, avail;

    if (!rb || !data || len == 0)
        return 0;

    avail = rb->capacity - rb->sz;
    if (len > avail)
        n = avail;
    else
        n = len;

    /* First segment: from head to end of storage */
    size_t first = rb->capacity - rb->head;

    if (n <= first) {
        memcpy(rb->storage + rb->head, data, n);
        rb->head = (rb->head + n) % rb->capacity;
    } else {
        memcpy(rb->storage + rb->head, data, first);
        memcpy(rb->storage, data + first, n - first);
        rb->head = n - first;
    }

    rb->sz += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    if (len > rb->sz)
        n = rb->sz;
    else
        n = len;

    /* First segment: from tail to end of storage */
    size_t first = rb->capacity - rb->tail;

    if (n <= first) {
        memcpy(out, rb->storage + rb->tail, n);
        rb->tail = (rb->tail + n) % rb->capacity;
    } else {
        memcpy(out, rb->storage + rb->tail, first);
        memcpy(out + first, rb->storage, n - first);
        rb->tail = n - first;
    }

    rb->sz -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n;

    if (!rb || !out || len == 0)
        return 0;

    if (len > rb->sz)
        n = rb->sz;
    else
        n = len;

    /* Copy from tail without advancing */
    size_t first = rb->capacity - rb->tail;

    if (n <= first) {
        memcpy(out, rb->storage + rb->tail, n);
    } else {
        memcpy(out, rb->storage + rb->tail, first);
        memcpy(out + first, rb->storage, n - first);
    }

    return n;
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
