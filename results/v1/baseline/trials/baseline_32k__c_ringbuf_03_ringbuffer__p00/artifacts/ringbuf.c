#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t used;
};

ringbuf_t *rb_create(size_t cap) {
    if (cap == 0) return NULL;
    ringbuf_t *rb = calloc(1, sizeof(*rb));
    if (!rb) return NULL;
    rb->storage = calloc(cap, sizeof(uint8_t));
    if (!rb->storage) { free(rb); return NULL; }
    rb->capacity = cap;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->storage);
    free(rb);
}

size_t rb_size(const ringbuf_t *rb) {
    return rb ? rb->used : 0;
}

size_t rb_capacity(const ringbuf_t *rb) {
    return rb ? rb->capacity : 0;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->used = 0;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    size_t free_space = rb->capacity - rb->used;
    if (free_space == 0) return 0;

    /* Cap writes to available space */
    size_t nw = len < free_space ? len : free_space;

    /* First write segment (until end of buffer, or all bytes) */
    size_t n1 = ((rb->capacity - rb->head)) < nw ?
        (rb->capacity - rb->head) : nw;
    memcpy(rb->storage + rb->head, data, n1);

    /* Wrap-around segment */
    if (n1 < nw) {
        size_t n2 = nw - n1;
        memcpy(rb->storage, data + n1, n2);
    }

    rb->head = (rb->head + nw) % rb->capacity;
    rb->used += nw;
    return nw;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->used == 0) return 0;

    size_t nr = rb->used < len ? rb->used : len;

    /* First read segment (until end of buffer, or all bytes needed) */
    size_t n1 = ((rb->capacity - rb->tail)) < nr ?
        (rb->capacity - rb->tail) : nr;
    memcpy(out, rb->storage + rb->tail, n1);

    /* Wrap-around segment */
    if (n1 < nr) {
        size_t n2 = nr - n1;
        memcpy(out + n1, rb->storage, n2);
    }

    rb->tail = (rb->tail + nr) % rb->capacity;
    rb->used -= nr;
    return nr;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->used == 0) return 0;

    size_t np = rb->used < len ? rb->used : len;

    /* First peek segment */
    size_t n1 = ((rb->capacity - rb->tail)) < np ?
        (rb->capacity - rb->tail) : np;
    memcpy(out, rb->storage + rb->tail, n1);

    /* Wrap-around segment */
    if (n1 < np) {
        size_t n2 = np - n1;
        memcpy(out + n1, rb->storage, n2);
    }
    return np;
}
