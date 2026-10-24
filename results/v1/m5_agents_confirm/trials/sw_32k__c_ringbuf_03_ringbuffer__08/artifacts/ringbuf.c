#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
};

ringbuf_t *rb_create(size_t capacity) {
    ringbuf_t *rb;
    if (capacity == 0)
        return NULL;
    rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;
    rb->data = calloc(capacity, sizeof(uint8_t));
    if (!rb->data) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->data);
    free(rb);
}

static size_t wrap_write(ringbuf_t *rb, const uint8_t *src, size_t n,
                          size_t dest) {
    size_t i;
    for (i = 0; i < n; i++) {
        rb->data[dest] = src[i];
        dest = (dest + 1) % rb->capacity;
    }
    return i;
}

static size_t wrap_read(ringbuf_t *rb, uint8_t *dst, size_t n, size_t src) {
    size_t i;
    for (i = 0; i < n; i++) {
        dst[i] = rb->data[src];
        src = (src + 1) % rb->capacity;
    }
    return i;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len) {
    size_t written, avail, want;
    size_t to_cap_end;

    if (!rb || !data || len == 0)
        return 0;

    for (written = 0; written < len;) {
        want = len - written;
        avail = rb->capacity - rb->count;
        if (avail == 0)
            break;
        if (want > avail)
            want = avail;
        to_cap_end = rb->capacity - rb->tail;
        if (to_cap_end > want)
            to_cap_end = want;
        wrap_write(rb, data + written, to_cap_end, rb->tail);
        rb->tail = (rb->tail + to_cap_end) % rb->capacity;
        rb->count += to_cap_end;
        written += to_cap_end;
    }
    return written;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len) {
    size_t read_back, want;
    size_t to_cap_end;

    if (!rb || !out || len == 0)
        return 0;

    for (read_back = 0; read_back < len;) {
        if (rb->count == 0)
            break;
        want = len - read_back;
        to_cap_end = rb->capacity - rb->head;
        if (to_cap_end > rb->count)
            to_cap_end = rb->count;
        if (to_cap_end > want)
            to_cap_end = want;
        wrap_read(rb, out + read_back, to_cap_end, rb->head);
        rb->head = (rb->head + to_cap_end) % rb->capacity;
        rb->count -= to_cap_end;
        read_back += to_cap_end;
    }
    return read_back;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len) {
    size_t peeked, want;
    size_t to_cap_end;
    size_t src;

    if (!rb || !out || len == 0)
        return 0;

    for (peeked = 0; peeked < len;) {
        if (rb->count == 0)
            break;
        want = len - peeked;
        to_cap_end = rb->capacity - rb->head;
        if (to_cap_end > rb->count)
            to_cap_end = rb->count;
        if (to_cap_end > want)
            to_cap_end = want;
        for (src = rb->head; src < rb->head + to_cap_end; src++)
            out[peeked++] = rb->data[src % rb->capacity];
    }
    return peeked;
}

size_t rb_size(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb) {
    if (!rb)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb) {
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
