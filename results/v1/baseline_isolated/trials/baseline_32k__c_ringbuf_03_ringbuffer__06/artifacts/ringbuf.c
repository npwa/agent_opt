#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;     /* allocated storage                */
    size_t   capacity;/* total slots                      */
    size_t   head;    /* offset of oldest byte            */
    size_t   tail;    /* offset where next write goes     */
    size_t   count;   /* bytes currently stored             */
};

ringbuf_t *rb_create(size_t cap) {
    if (cap == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    uint8_t *p = calloc(cap, sizeof(uint8_t));
    if (!p) { free(rb); return NULL; }
    rb->buf     = p;
    rb->capacity = cap;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    if (rb->count == rb->capacity) return 0;
    size_t free_space = rb->capacity - rb->count;
    size_t n          = len < free_space ? len : free_space;

    /* forward copy from tail toward end of buffer */
    size_t avail      = rb->capacity - rb->tail;
    size_t take       = n < avail ? n : avail;
    memcpy(rb->buf + rb->tail, data, take);

    if (take != n) {
        /* wrap around to beginning of buffer for remaining bytes */
        memcpy(rb->buf, data + take, n - take);
    }

    rb->tail  = (rb->tail + n) % rb->capacity;
    rb->count += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->count == 0)          return 0;
    size_t n = rb->count < len ? rb->count : len;

    /* forward copy from head toward end of buffer */
    size_t avail = rb->capacity - rb->head;
    size_t take  = n < avail ? n : avail;
    memcpy(out, rb->buf + rb->head, take);

    if (take != n) {
        /* wrap around: read from beginning of buffer for remaining bytes */
        memcpy(out + take, rb->buf + rb->head + take, n - take);
    }

    rb->head  = (rb->head + n) % rb->capacity;
    rb->count -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->count == 0)          return 0;
    size_t n = rb->count < len ? rb->count : len;

    /* forward copy from head toward end of buffer */
    size_t avail = rb->capacity - rb->head;
    size_t take  = n < avail ? n : avail;
    memcpy(out, rb->buf + rb->head, take);

    if (take != n) {
        /* wrap around: read from beginning of buffer for remaining bytes */
        memcpy(out + take, rb->buf + rb->head + take, n - take);
    }

    return n;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
