#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

#define RINGBUF_MAGIC 0x52494E47U /* "RING" */

struct ringbuf {
    uint8_t *buf;
    size_t   head;        /* oldest byte (front) */
    size_t   tail;        /* next free slot after newest byte */
    size_t   capacity;
    unsigned int magic;
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
    rb->capacity  = capacity;
    rb->head      = 0;
    rb->tail      = 0;
    rb->magic     = RINGBUF_MAGIC;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->buf);
    rb->magic = 0;
    free(rb);
}

static size_t count(const ringbuf_t *rb) {
    /* bytes stored == wrapped distance from head to tail */
    if (rb->tail >= rb->head)
        return rb->tail - rb->head;
    return rb->capacity - rb->head + rb->tail;
}

size_t rb_size(const ringbuf_t *rb) {
    return rb ? count(rb) : 0;
}

size_t rb_capacity(const ringbuf_t *rb) {
    return rb ? rb->capacity : 0;
}

/* Copy bytes from the ring buffer region starting at `start` (wrapping
   through capacity) into a flat buffer.  Returns the actual number of
   bytes written to *out. */
static void flat_write_from(ringbuf_t const *rb,
                            size_t          start,
                            size_t          max_bytes,
                            uint8_t        *out)
{
    if (max_bytes == 0)
        return;

    /* First chunk: from `start` to end of storage. */
    size_t first = rb->capacity - start;
    if (first <= max_bytes) {
        memcpy(out, rb->buf + start, first);
        out += first;
        max_bytes -= first;
        /* Wraps around: second chunk starts at index 0. */
        memcpy(out, rb->buf, max_bytes);       /* always fits by invariant */
    } else {
        memcpy(out, rb->buf + start, max_bytes);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0)
        return 0;

    /* How many bytes can still be stored? */
    size_t free_space = count(rb) >= rb->capacity ? 0
                        : rb->capacity - count(rb);
    if (free_space == 0)
        return 0;

    size_t n = len < free_space ? len : free_space;   /* cap to what fits */

    const uint8_t *p = data;
    size_t remaining = n;

    while (remaining > 0) {
        /* --- First chunk: start at tail, advance toward end of storage --- */
        size_t first = rb->capacity - rb->tail;          /* free slots to end */
        size_t write1 = remaining < first ? remaining : first;

        memcpy(rb->buf + rb->tail, p, write1);
        p           += write1;
        remaining   -= write1;

        if (remaining == 0)
            break;                                       /* done in one chunk */

        /* First chunk exhausted its slots → tail reached capacity. */
    }

    free_space = count(rb) >= rb->capacity ? 0
                 : rb->capacity - count(rb);
    if (free_space == 0)
        return n;                                      /* no room before wrapping */

    /* --- Wrap-around: second chunk at the front, up to but not past head --- */
    size_t remaining_after_first = free_space < rb->head ? free_space : rb->head;

    if (remaining_after_first > 0) {                   /* there is truly a region [0 .. some] free */
        flat_write_from(rb, rb->tail, remaining_after_first, p);
        rb->tail = count(rb) >= rb->capacity ? 0
                   : rb->capacity - count(rb);          /* new tail position */
    }

    return n;
}


size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;

    size_t sz = count(rb);
    if (sz == 0)
        return 0;

    size_t n = len < sz ? len : sz;                  /* cap to what's stored */

    flat_write_from(rb, rb->head, n, out);            /* oldest bytes at head */

    /* Advance head.  After copying `n` bytes from head: */
    if (rb->head + n >= rb->capacity)                 /* wrapped past end? */
        rb->head = rb->head + n - rb->capacity;       /* wrap position in ring */
    else
        rb->head += n;

    if (count(rb) == 0)                              /* buffer now empty? */
        rb->head = rb->tail = 0;                      /* normalize */

    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0)
        return 0;

    size_t sz = count(rb);
    if (sz == 0)
        return 0;

    size_t n = len < sz ? len : sz;                  /* cap to what's stored */
    flat_write_from(rb, rb->head, n, out);           /* same layout as read */
    return n;                                        /* no head advance!             */
}

void rb_clear(ringbuf_t *rb) {
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
}
