#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

#define RINGBUF_MAGIC 0x524642FA  /* "RBF\037" */

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;       /* next byte to read from     */
    size_t tail;       /* next free slot for writing   */
    size_t sz;         /* current number of bytes      */
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0)
        return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;
    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->sz       = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

/* Write up to `len` bytes from `data`.
   Returns the number actually written (0 on NULL / full). */
size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || rb->sz == rb->capacity)
        return 0;
    size_t free = rb->capacity - rb->sz;
    if (len > free)
        len = free;
    for (size_t i = 0; i < len; ++i) {
        rb->buf[rb->tail] = data[i];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
    rb->sz += len;
    return len;
}

/* Read up to `len` bytes into `out`, oldest first.
   Returns the number actually read (0 on NULL / empty). */
size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || rb->sz == 0)
        return 0;
    if (len > rb->sz)
        len = rb->sz;
    for (size_t i = 0; i < len; ++i) {
        out[i] = rb->buf[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
    }
    rb->sz -= len;
    return len;
}

/* Copy up to `len` oldest bytes into `out` without removing them. */
size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || rb->sz == 0)
        return 0;
    if (len > rb->sz)
        len = rb->sz;
    for (size_t i = 0; i < len; ++i) {
        out[i] = rb->buf[(rb->head + i) % rb->capacity];
    }
    return len;
}

/* Number of bytes currently stored. */
size_t rb_size(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->sz;
}

/* Total capacity given at creation. */
size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->capacity;
}

/* Discard all content. */
void rb_clear(ringbuf_t *rb) {
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->sz   = 0;
}
