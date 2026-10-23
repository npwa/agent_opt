#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;     /* write index (overwrites oldest on full) */
    size_t tail;     /* read index                                */
    size_t size;     /* bytes currently stored                    */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = calloc((size_t)1, capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->size     = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

/* Write up to `len` bytes into the buffer at `head`.
 * Only writes what fits (max capacity - size).
 * Returns actual bytes written. */
size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    /* How many slots are free? */
    size_t n = rb->size < rb->capacity ? rb->capacity - rb->size : 0;
    if (n == 0 || len == 0)
        return 0;

    if (len < n)
        n = len;   /* write all of it */

    /* Copy from head to end, wrap around. */
    size_t chunk_size = rb->capacity - rb->head;
    if (chunk_size >= n) {
        memcpy(rb->buf + rb->head, data, n);
        rb->head = rb->head + n;               /* safe -- never exceeds capacity */
    } else {
        memcpy(rb->buf + rb->head, data, chunk_size);  /* to end   */
        size_t remainder = n - chunk_size;
        memcpy(rb->buf, data + chunk_size, remainder);   /* from start */
        rb->head = remainder;                  /* head now points at next write slot   */
    }

    rb->size += n;
    return n;
}

/* Read up to `len` bytes from the buffer at `tail`, oldest first.
 * Returns actual bytes read (never more than available or requested). */
size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    /* Never read more than is currently in the buffer. */
    size_t n = rb->size < len ? rb->size : len;
    if (n == 0)
        return 0;

    /* Split at the end-of-buffer boundary, wrapping around. */
    size_t chunk_size = rb->capacity - rb->tail;
    if (chunk_size >= n) {
        memcpy(out, rb->buf + rb->tail, n);
        rb->tail = rb->tail + n;            /* safe -- never exceeds capacity */
    } else {
        memcpy(out, rb->buf + rb->tail, chunk_size);  /* to end   */
        size_t remainder = n - chunk_size;
        memcpy(out + chunk_size, rb->buf, remainder);   /* from start */
        rb->tail = remainder;                  /* tail now points at next read slot  */
    }

    rb->size -= n;
    return n;
}

/* Copy (peek at) up to `len` oldest bytes without consuming them.
 * Always leaves the buffer unchanged. */
size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    /* Clamp to available bytes without mutating the struct. */
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
    size_t n = rb->size < len ? rb->size : len;
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif

    if (n == 0)
        return 0;

    /* Use a snapshot of tail so the compiler can't assume &rb->tail is stable. */
    size_t t = rb->tail;

    size_t chunk_size = rb->capacity - t;
    if (chunk_size >= n) {
        memcpy(out, rb->buf + t, n);
    } else {
        memcpy(out, rb->buf + t, chunk_size);  /* to end   */
        size_t remainder = n - chunk_size;
        memcpy(out + chunk_size, rb->buf, remainder);   /* from start */
    }

    return n;
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
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
