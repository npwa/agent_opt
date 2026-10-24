#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *storage; /* internal circular buffer */
    size_t capacity;  /* total slots allocated (may be > logical capacity) */
    size_t head;      /* index of oldest byte */
    size_t tail;      /* index where next write goes */
    size_t count;     /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb) return NULL;
    rb->storage = calloc(capacity, sizeof(uint8_t));
    if (!rb->storage) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    /* how many slots are free */
    size_t avail = rb->capacity - rb->count;
    size_t to_write = len < avail ? len : avail;

    for (size_t i = 0; i < to_write; i++) {
        rb->storage[rb->tail] = data[i];
        rb->tail++;
        if (rb->tail == rb->capacity) rb->tail = 0; /* wrap */
    }
    rb->count += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    size_t to_read = len < rb->count ? len : rb->count;

    for (size_t i = 0; i < to_read; i++) {
        out[i] = rb->storage[rb->head];
        rb->head++;
        if (rb->head == rb->capacity) rb->head = 0; /* wrap */
    }
    rb->count -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    size_t to_peek = len < rb->count ? len : rb->count;

    size_t h = rb->head;
    for (size_t i = 0; i < to_peek; i++) {
        out[i] = rb->storage[h];
        h++;
        if (h == rb->capacity) h = 0;
    }
    return to_peek;
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
