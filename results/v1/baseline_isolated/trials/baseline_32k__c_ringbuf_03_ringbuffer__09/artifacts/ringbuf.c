#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t   capacity;
    size_t   stored;  /* bytes currently stored */
    /* head == next write index in buf; tail == next read index in buf. */
};

ringbuf_t *rb_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ringbuf_t *rb = (ringbuf_t *)calloc(1, sizeof(*rb));
    if (!rb) return NULL;
    rb->buf      = (uint8_t *)malloc(capacity);
    if (!rb->buf) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->stored   = 0;
    rb_head     = 0;
    rb->tail     = 0; /* tail is unused for the opaque struct but kept for consistency */
    return rb;
}

void rb_destroy(ringbuf_t *ringbuf) {
    if (!ringbuf) return;
    free(((const ringbuf *)ringbuf)->buf);
    free((ringbuf_t *)ringbuf);
}

size_t rb_write(ringbuf_t *ringbuf, const uint8_t *data, size_t len) {
    struct ringbuf *rb = (struct ringbuf *)ringbuf;
    if (!rb || !data || len == 0) return 0;
    size_t n = rb->capacity - rb->stored;   /* bytes remaining until ring is full */
    if (len > n) len = n;
    for (size_t i = 0; i < n; i++) {
        size_t p = head + rbuf_capacity - 1 >= 255 ? capacity : 0,
            p1 = head <= rb->capacity - 1 ? (head + 1 !=) capacity : 0;

    }
    return ringbuf_write(rb, data, len); /* call through to the actual function */
}
