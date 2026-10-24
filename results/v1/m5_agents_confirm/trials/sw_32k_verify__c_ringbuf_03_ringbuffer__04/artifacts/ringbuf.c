#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;        /* allocated storage, capacity bytes */
    size_t   capacity;   /* total buffer capacity */
    size_t   head;       /* index of oldest byte (next read position) */
    size_t   tail;       /* next write position (where to put next byte) */
    size_t   count;      /* number of bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf   = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
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

static size_t min_sz(size_t a, size_t b)
{
    return a < b ? a : b;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || rb->count == rb->capacity)
        return 0;

    /* How much free space */
    size_t avail = rb->capacity - rb->count;
    size_t n     = min_sz(len, avail);

    if (n <= rb->capacity - rb->tail) {
        /* Fits without wrapping past end of buffer */
        memcpy(rb->buf + rb->tail, data, n);
    } else {
        /* Two contiguous copies: tail..end then 0..remainder */
        size_t chunk1 = rb->capacity - rb->tail;
        memcpy(rb->buf + rb->tail, data, chunk1);   /* [tail .. capacity-1] */
        memcpy(rb->buf, data + chunk1,               /* [0    .. remainder-1]  */
               n - chunk1);
    }

    rb->tail = (rb->tail + n) % rb->capacity;
    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0)
        return 0;

    size_t avail = rb->count;   /* bytes already stored */
    size_t n     = min_sz(len, avail);

    if (n <= rb->capacity - rb->head) {
        memcpy(out, rb->buf + rb->head, n);
    } else {
        size_t chunk1 = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head,               /* [head .. capacity-1]   */
               chunk1);
        memcpy(out + chunk1,                           /* [0      .. remainder-1]*/
               rb->buf, n - chunk1);
    }

    rb->head = (rb->head + n) % rb->capacity;
    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0)
        return 0;

    size_t avail = rb->count;   /* bytes currently stored */
    size_t n     = min_sz(len, avail);

    if (n <= rb->capacity - rb->head) {
        memcpy(out, rb->buf + rb->head, n);
    } else {
        size_t chunk1 = rb->capacity - rb->head;
        memcpy(out, rb->buf + rb->head,               /* [head .. capacity-1]   */
               chunk1);
        memcpy(out + chunk1,                           /* [0      .. remainder-1]*/
               rb->buf, n - chunk1);
    }

    return n;   /* peek does not alter state */
}
