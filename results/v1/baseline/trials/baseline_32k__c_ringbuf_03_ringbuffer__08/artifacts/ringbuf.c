#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t size;
    size_t head; /* next write position */
};

static void advance(ringbuf_t *rb, size_t *pos) {
    *pos = (*pos + 1) % rb->capacity;
}

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;

    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->head = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb) return;
    free(rb->buf);
    free(rb);
}

static size_t write_pos(const ringbuf_t *rb, size_t offset) {
    return (rb->head + offset) % rb->capacity;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) return 0;

    size_t avail = rb->capacity - rb->size;
    if (avail == 0) return 0;

    size_t written = len > avail ? avail : len;

    /* Write in two segments to handle wrap-around */
    for (size_t i = 0; i < written; ) {
        size_t end_segment = rb->capacity - rb->head;
        size_t segment_len = (written - i) < end_segment ? (written - i) : end_segment;

        uint8_t *dst = &rb->buf[rb->head];
        const uint8_t *src = &data[i];
        for (size_t j = 0; j < segment_len; j++) {
            dst[j] = src[j];
        }

        rb->head += segment_len;
        if (rb->head >= rb->capacity) rb->head -= rb->capacity;
        i += segment_len;
    }

    rb->size += written;
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->size == 0) return 0;

    size_t read_count = len > rb->size ? rb->size : len;
    size_t tail = (rb->head + rb->capacity - rb->size) % rb->capacity;

    for (size_t i = 0; i < read_count; ) {
        size_t end_segment = rb->capacity - tail;
        size_t segment_len = (read_count - i) < end_segment ? (read_count - i) : end_segment;

        uint8_t *dst = &out[i];
        const uint8_t *src = &rb->buf[tail];
        for (size_t j = 0; j < segment_len; j++) {
            dst[j] = src[j];
        }

        tail += segment_len;
        if (tail >= rb->capacity) tail -= rb->capacity;
        i += segment_len;
    }

    rb->size -= read_count;
    /* Advance head by read_count */
    rb->head += read_count;
    while (rb->head >= rb->capacity) rb->head -= rb->capacity;

    return read_count;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) return 0;
    if (rb->size == 0) return 0;

    size_t peek_count = len > rb->size ? rb->size : len;
    size_t tail = (rb->head + rb->capacity - rb->size) % rb->capacity;

    for (size_t i = 0; i < peek_count; ) {
        size_t end_segment = rb->capacity - tail;
        size_t segment_len = (peek_count - i) < end_segment ? (peek_count - i) : end_segment;

        uint8_t *dst = &out[i];
        const uint8_t *src = &rb->buf[tail];
        for (size_t j = 0; j < segment_len; j++) {
            dst[j] = src[j];
        }

        tail += segment_len;
        if (tail >= rb->capacity) tail -= rb->capacity;
        i += segment_len;
    }

    return peek_count;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->size;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb) return;
    rb->size = 0;
    rb->head = 0;
}
