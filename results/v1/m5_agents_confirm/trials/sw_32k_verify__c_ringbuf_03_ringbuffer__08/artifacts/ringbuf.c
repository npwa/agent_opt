#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;   /* total slots (always > 0) */
    size_t head;       /* next write index */
    size_t tail;       /* next read (oldest) index */
    size_t count;      /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
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
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
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

/* Write `n` consecutive bytes into the buffer starting at head, wrapping.
   Returns how many of those n actually fit (<= n). Caller must not pass
   a second segment when wrap-around occurs. */
static size_t write_wrap(ringbuf_t *rb, const uint8_t *data, size_t off, size_t n)
{
    /* Write first chunk up to end-of-array */
    size_t space_to_end = rb->capacity - rb->head;
    size_t first = (n < space_to_end) ? n : space_to_end;

    for (size_t j = 0; j < first; ++j)
        rb->buf[rb->head + j] = data[off + j];
    rb->head += first;
    if (rb->head == rb->capacity)
        rb->head = 0;

    /* Remaining bytes wrap to start */
    for (size_t j = 0; j < n - first; ++j)
        rb->buf[j] = data[off + first + j];
    rb->head += n - first;
    return n;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0 || rb->count == rb->capacity)
        return 0;

    size_t avail = rb->capacity - rb->count;   /* free slots */
    size_t n = (len < avail) ? len : avail;     /* at most what fits */

    write_wrap(rb, data, 0, n);
    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t n = (len < rb->count) ? len : rb->count;

    /* Read first chunk up to end-of-array */
    size_t space_to_end = rb->capacity - rb->tail;
    size_t first = (n < space_to_end) ? n : space_to_end;

    for (size_t j = 0; j < first; ++j) {
        out[j] = rb->buf[rb->tail + j];
    }
    rb->tail += first;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    /* Remaining bytes wrap to start */
    for (size_t j = 0; j < n - first; ++j) {
        out[first + j] = rb->buf[j];
    }
    rb->tail += n - first;

    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t n = (len < rb->count) ? len : rb->count;

    /* Peek first chunk up to end-of-array */
    size_t space_to_end = rb->capacity - rb->tail;
    size_t first = (n < space_to_end) ? n : space_to_end;

    for (size_t j = 0; j < first; ++j) {
        out[j] = rb->buf[rb->tail + j];
    }

    /* Remaining bytes wrap to start */
    for (size_t j = 0; j < n - first; ++j) {
        out[first + j] = rb->buf[j];
    }

    return n;
}
