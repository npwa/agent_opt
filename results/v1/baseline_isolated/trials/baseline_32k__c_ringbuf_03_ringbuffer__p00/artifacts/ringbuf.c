#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *buf;
    size_t  cap;
    size_t  sz;
    size_t  rd;
    size_t  wr;
};

static inline size_t inc(size_t v, size_t lim)
{
    ++v;
    if (v >= lim)
        return 0;
    return v;
}

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *r = malloc(sizeof(*r));
    if (!r) return NULL;
    r->buf = calloc(capacity, sizeof(uint8_t));
    if (!r->buf) { free(r); return NULL; }
    r->cap = capacity;
    r->sz  = 0;
    r->rd  = 0;
    r->wr  = 0;
    return r;
}

void rb_destroy(ringbuf_t *r)
{
    if (!r) return;
    free(r->buf);
    free(r);
}

size_t rb_write(ringbuf_t *r, const uint8_t *data, size_t len)
{
    if (!r || !data || r->sz == r->cap || len == 0)
        return 0;

    size_t avail = r->cap - r->sz;
    size_t n     = len < avail ? len : avail;

    for (size_t i = 0; i < n; i++) {
        r->buf[r->wr] = data[i];
        r->wr = inc(r->wr, r->cap);
    }
    r->sz += n;
    return n;
}

size_t rb_read(ringbuf_t *r, uint8_t *out, size_t len)
{
    if (!r || !out || r->sz == 0)
        return 0;

    size_t n = len < r->sz ? len : r->sz;

    for (size_t i = 0; i < n; i++) {
        out[i] = r->buf[r->rd];
        r->rd = inc(r->rd, r->cap);
    }
    r->sz -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *r, uint8_t *out, size_t len)
{
    if (!r || !out || r->sz == 0)
        return 0;

    size_t lim = len < r->sz ? len : r->sz;
    size_t c   = 0;
    size_t p   = r->rd;

    for (size_t i = 0; i < lim; i++) {
        out[i]      = r->buf[p];
        p           = inc(p, r->cap);
        ++c;
    }
    return c;
}

size_t rb_size(const ringbuf_t *r)
{
    if (!r) return 0;
    return r->sz;
}

size_t rb_capacity(const ringbuf_t *r)
{
    if (!r) return 0;
    return r->cap;
}

void rb_clear(ringbuf_t *r)
{
    if (!r) return;
    r->sz = 0;
    r->rd = 0;
    r->wr = 0;
}
