#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    rb->buf = aligned_alloc(1, capacity);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    rb->capacity = capacity;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (rb) {
        free(rb->buf);
        free(rb);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    size_t free = rb->capacity - rb->size;
    size_t n = len > free ? free : len;
    size_t cnt = 0;
    while (cnt < n) {
        size_t avail = (rb->tail + rb->capacity - rb->head) % rb->capacity;
        size_t write = avail > (n - cnt) ? (n - cnt) : avail;
        memcpy(rb->buf + rb->tail, data + cnt, write);
        rb->tail = (rb->tail + write) % rb->capacity;
        cnt += write;
    }
    rb->size = (rb->size + cnt) % rb->capacity;
    return cnt;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    size_t cnt = 0;
    while (cnt < len) {
        size_t avail = rb->size - cnt;
        size_t read = avail > (len - cnt) ? (len - cnt) : avail;
        memcpy(out + cnt, rb->buf + rb->head, read);
        rb->head = (rb->head + read) % rb->capacity;
        cnt += read;
    }
    rb->size = (rb->size - cnt) % rb->capacity;
    return cnt;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    size_t cnt = 0;
    while (cnt < len) {
        size_t avail = rb->size - cnt;
        size_t read = avail > (len - cnt) ? (len - cnt) : avail;
        memcpy(out + cnt, rb->buf + rb->head, read);
        cnt += read;
    }
    return cnt;
}

size_t rb_size(const ringbuf_t *rb) {
    return rb ? rb->size : 0;
}

size_t rb_capacity(const ringbuf_t *rb) {
    return rb ? rb->capacity : 0;
}

void rb_clear(ringbuf_t *rb) {
    if (rb) {
        rb->head = 0;
        rb->tail = 0;
        rb->size = 0;
    }
}