#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t sz;
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    rb->storage = calloc(capacity, sizeof(uint8_t));
    if (!rb->storage) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->sz   = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb) return;
    free(rb->storage);
    free(rb);
}

static size_t do_write_chunk(ringbuf_t *rb, size_t n)
{
    size_t remain = rb->capacity - rb->tail;
    size_t chunk  = (remain < n) ? remain : n;
    rb->tail += chunk;
    if (rb->tail == rb->capacity) rb->tail = 0;
    rb->sz += chunk;
    return chunk;
}

static size_t do_read_chunk(ringbuf_t *rb, size_t n)
{
    size_t remain = rb->capacity - rb->head;
    size_t chunk  = (remain < n) ? remain : n;
    rb->head += chunk;
    if (rb->head == rb->capacity) rb->head = 0;
    rb->sz -= chunk;
    return chunk;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t nwritten = 0;
    if (!rb || !data || rb->sz == rb->capacity) return 0;
    while (nwritten < len && rb->sz < rb->capacity) {
        size_t chunk;
        if (nwritten > 0) {
            chunk = do_write_chunk(rb, len - nwritten);
            memcpy(rb->storage + rb->head, data + nwritten, chunk);
            rb->tail += chunk;
            if (rb->tail == rb->capacity) rb->tail = 0;
            rb->sz += chunk;
        }
        /* WRONG approach */
    }
    return nwritten;
}

