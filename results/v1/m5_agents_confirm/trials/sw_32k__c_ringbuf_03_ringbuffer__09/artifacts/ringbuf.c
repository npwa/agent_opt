#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *store;
    size_t capacity;
    size_t head;  /* write index */
    size_t tail;  /* read index */
    size_t count; /* current bytes stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->store = malloc(capacity);
    if (!rb->store) {
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
    free(rb->store);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t nwritten;

    if (!rb || !data || len == 0)
        return 0;

    nwritten = len;
    if (nwritten > rb->capacity - rb->count)
        nwritten = rb->capacity - rb->count;

    {
        size_t to_end = rb->capacity - rb->head;

        if (nwritten <= to_end) {
            memcpy(&rb->store[rb->head], data, nwritten);
            rb->head += nwritten;
        } else {
            memcpy(&rb->store[rb->head], data, to_end);
            rb->head = 0;
            /* remaining */
            nwritten -= to_end;
            memcpy(rb->store, data + to_end, nwritten);
            rb->head = nwritten;
        }
    }

    if (rb->head == rb->capacity)
        rb->head = 0;

    rb->count += nwritten;
    return nwritten;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t nread;

    if (!rb || !out || len == 0)
        return 0;

    nread = len;
    if (nread > rb->count)
        nread = rb->count;

    {
        size_t to_end = rb->capacity - rb->tail;

        if (nread <= to_end) {
            memcpy(out, &rb->store[rb->tail], nread);
            rb->tail += nread;
        } else {
            memcpy(out, &rb->store[rb->tail], to_end);
            rb->tail = 0;
            /* remaining */
            nread -= to_end;
            memcpy(out + to_end, rb->store, nread);
            rb->tail = nread;
        }
    }

    if (rb->tail == rb->capacity)
        rb->tail = 0;

    rb->count -= nread;
    return nread;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t npeeked;
    size_t to_end;

    if (!rb || !out || len == 0)
        return 0;

    npeeked = len;
    if (npeeked > rb->count)
        npeeked = rb->count;

    to_end = rb->capacity - rb->tail;

    if (npeeked <= to_end) {
        memcpy(out, &rb->store[rb->tail], npeeked);
    } else {
        memcpy(out, &rb->store[rb->tail], to_end);
        memcpy(out + to_end, rb->store, npeeked - to_end);
    }

    return npeeked;
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
