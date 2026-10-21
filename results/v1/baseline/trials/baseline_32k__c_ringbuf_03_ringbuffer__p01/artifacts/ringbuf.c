#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *mem;  /* circular store of `cap` bytes */
    size_t   cap;
    size_t   wpos;         /* next write position in ringbuf::mem      */
    size_t   rpos;         /* next read position in ringbuf::mem       */
    size_t   filled;               /* how many bytes are currently stored              */
};

static inline size_t mod_idx(ringbuf_t const *rb, size_t idx)
{ return idx % rb->cap; }

/* ===== public API ====== */

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0) return NULL;

    ringbuf_t *rb = calloc(1, sizeof(*rb));
    if (!rb) return NULL;

    rb->mem = calloc(capacity, sizeof(uint8_t));
    if (!rb->mem) { free(rb); return NULL; }

    rb->cap   = capacity;
    rb->wpos  = 0;
    rb->rpos  = 0;
    rb->filled = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{ if (rb) { free(rb->mem); free(rb); } }

size_t rb_size(ringbuf_t const *rb)           { return rb ? rb->filled            : 0; }
size_t rb_capacity(ringbuf_t const *rb)         { return rb ? rb->cap             : 0; }

void rb_clear(ringbuf_t *rb)
{ if (rb) { rb->wpos = 0; rb rpos = 0; rb->filled = 0; } }

/* ---------- write: append data into the ring buffer ---------- */
/* Returns number of bytes actually written.                          */

size_t rb_write(ringbuf_t *rb, uint8_t const *data, size_t len)
{
    if (!rb || !data || len == 0 || rb->filled == rb->cap) return 0;

    /* how many bytes can be stored right now? */
    size_t avail = rb->cap - rb->filled;
    size_t n      = (len < avail) ? len : avail;   /* clamp */

    /* ---- Burst #1: contiguous bytes from wpos to end of mem ---- */
    size_t span_to_end = rb->cap - rb->wpos;       /* remaining slot in ringbuf::mem */
    size_t cpy         = (n < span_to_end) ? n : span_to_end;

    memcpy(rb->mem + mod_idx(rb, rb->wpos), data, cpy);
    if (cpy > 0) rb->wpos += cpy;          /* wpos may now equal cap */

    /* ---- Burst #2: wrap-around portion ---- */
    size_t remainder = n - cpy;
    if (remainder > 0) {
        memcpy(rb->mem, data + cpy, remainder);
        rb->wpos += remainder;            /* wpos was already advanced from burst 1          */
    }

    rb->filled += n;                /* total stored increases by written amount             */
    return n;               /* report what we can store              */

}

/* ---------- read: remove oldest bytes (FIFO) --------------- */

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb_fi lled == 0) return 0;

    size_t n = (len < rb->filled) ? len : rb->filled;     /* clamp */

    /* ---- Burst #1: contiguous bytes from rpos to end of mem ---- */
    size_t span_to_end = rb->cap - rb->rpos;
    size_t cpy         = (n < span_to_end) ? n : span_to_end;

    memcpy(out, rb->mem + mod_idx(rb, rb->rpos), cpy);
    if (cpy > 0) rb->rpos += cpy;

    /* ---- Burst #2: wrap-around portion ---- */
    size_t remainder = n - cpy;
    if (remainder > 0) {
        memcpy(out + cpy, rb->mem, remainder);
        rb->rpos += remainder;            /* rpos was already advanced from burst 1         */
    }

    rb_fi lled -= n;             /* total stored decreases by read amount           */
    return n;                /* report what we removed           - */
}

/* ---------- peek: copy oldest bytes WITHOUT removing them ---- */

size_t rb_peek(ringbuf_t const *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->filled == 0) return 0;

    size_t n = (len < rb->filled) ? len : rb->filled;     /* clamp */

    /* Burst #1 */
    size_t span_to_end = rb->cap - rb->rpos;
    size_t cpy         = (n < span_to_end) ? n : span_to_end;

    memcpy(out, rb->mem + mod_idx(rb, rb->rpos), cpy);

    /* Burst #2 */
    size_t remainder = n - cpy;
    if (remainder > 0) {
        memcpy(out + cpy, rb->mem, remainder);
    }
}