#include <stdlib.h>
#include <string.h>
#include "ringbuf.h"

typedef struct ringbuf {
    uint8_t *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
} ringbuf_t;

ringbuf_t *rb_create(size_t capacity) {
    if (capacity <= 0) {
        return NULL;
    }
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if (!rb) {
        return NULL;
    }
    rb->buffer = (uint8_t *)malloc(capacity);
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (rb) {
        free(rb->buffer);
        free(rb);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) {
        return 0;
    }
    size_t bytes_used = (rb->tail - rb->head + rb->capacity) % rb->capacity;
    size_t available = rb->capacity - bytes_used;
    size_t write_len = (len < available) ? len : available;
    if (write_len == 0) {
        return 0;
    }
    for (size_t i = 0; i < write_len; i++) {
        rb->buffer[rb->tail] = data[i];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
    return write_len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) {
        return 0;
    }
    size_t used = (rb->tail - rb->head) % rb->capacity;
    size_t read_len = (len < used) ? len : used;
    if (read_len == 0) {
        return 0;
    }
    for (size_t i = 0; i < read_len; i++) {
        out[i] = rb->buffer[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
    }
    return read_len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    if (!rb || !out || len == 0) {
        return 0;
    }
    size_t used = (rb->tail - rb->head) % rb->capacity;
    size_t read_len = (len < used) ? len : used;
    if (read_len == 0) {
        return 0;
    }
    for (size_t i = 0; i < read_len; i++) {
        out[i] = rb->buffer[rb->head];
    }
    return read_len;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    return (rb->tail - rb->head) % rb->capacity;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (rb) {
        rb->head = 0;
        rb->tail = 0;
    }
}