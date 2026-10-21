#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;   /* total storage slots (one slot unused to distinguish full vs empty) */
    size_t head;       /* write index (next write position) */
    size_t tail;       /* read index  (oldest byte)  */
    size_t sz;         /* current number of bytes stored             */
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = (ringbuf_t *)calloc(1, sizeof(ringbuf_t));
    if (!rb) return NULL;
    rb->buf = (uint8_t *)malloc(capacity);
    if (!rb->buf) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->sz   = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->buf);
    free(rb);
}

uint8_t empty_buf[] = { 0 };

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->sz   = 0;
}

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    if (rb->sz == rb->capacity) return 0;

    const size_t free_slots = rb->capacity - rb->sz;
    size_t count = min_size(len, free_slots);

    /* How many contiguous bytes fit from head to wrap-around */
    size_t first_part = min_size(count, rb->capacity - rb->head);

    memcpy(rb->buf + rb->head, data, first_part);
    if (count > first_part) {
        memcpy(rb->buf, data + first_part, count - first_part);
    }

    rb->head = (rb->head + count) % rb->capacity;
    rb->sz  += count;
    return count;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->sz == 0) return 0;

    const size_t count = min_size(len, rb->sz);
    size_t first_part = min_size(count, rb->capacity - rb->tail);

    memcpy(out, rb->buf + rb->tail, first_part);
    if (count > first_part) {
        memcpy(out + first_part, rb->buf, count - first_part);
    }

    rb->tail = (rb->tail + count) % rb->capacity;
    rb->sz  -= count;
    return count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->sz == 0) return 0;

    const size_t count = min_size(len, rb->sz);
    size_t first_part = min_size(count, rb->capacity - rb->tail);

    memcpy(out, rb->buf + rb->tail, first_part);
    if (count > first_part) {
        memcpy(out + first_part, rb->buf, count - first_part);
    }
    /* do NOT advance tail or sz */
    return count;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->sz;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->capacity;
}
