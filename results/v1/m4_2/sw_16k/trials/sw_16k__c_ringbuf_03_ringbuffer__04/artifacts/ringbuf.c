#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;   /* next read position */
    size_t tail;   /* next write position */
    size_t count;  /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb = NULL;

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

    rb->capacity  = capacity;
    rb->head      = 0;
    rb->tail      = 0;
    rb->count     = 0;

    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->storage);
    free(rb);
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

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t written = 0;
    size_t to_write;

    if (!rb || !data || len == 0)
        return 0;

    /* limit writes to available space */
    if (len > rb->capacity - rb->count)
        len = rb->capacity - rb->count;

    while (written < len) {
        size_t space_to_end = rb->capacity - rb->tail;
        size_t remaining    = len - written;

        to_write = (remaining < space_to_end) ? remaining : space_to_end;

        memcpy(rb->storage + rb->tail, data + written, to_write);
        rb->tail     += to_write;
        if (rb->tail == rb->capacity)
            rb->tail = 0;
        written    += to_write;
    }

    rb->count += written;
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t done = 0;
    size_t to_read;

    if (!rb || !out || len == 0)
        return 0;

    /* clamp to what's available */
    if (len > rb->count)
        len = rb->count;

    while (done < len) {
        size_t space_to_end = rb->capacity - rb->head;
        size_t remaining    = len - done;

        to_read = (remaining < space_to_end) ? remaining : space_to_end;

        memcpy(out + done, rb->storage + rb->head, to_read);
        rb->head     += to_read;
        if (rb->head == rb->capacity)
            rb->head = 0;
        rb->count   -= to_read;
        done        += to_read;
    }

    return done;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t done = 0;
    size_t to_read;
    size_t peek_head = rb->head;

    if (!rb || !out || len == 0)
        return 0;

    /* clamp to what's available */
    if (len > rb->count)
        len = rb->count;

    while (done < len) {
        size_t space_to_end = rb->capacity - peek_head;
        size_t remaining    = len - done;

        to_read = (remaining < space_to_end) ? remaining : space_to_end;

        memcpy(out + done, rb->storage + peek_head, to_read);
        peek_head += to_read;
        done     += to_read;
    }

    return done;
}
