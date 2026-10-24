#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;      /* write position */
    size_t tail;      /* read position */
    size_t count;     /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

static size_t min_sz(size_t a, size_t b) { return a < b ? a : b; }

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t free_space = rb->capacity - rb->count;
    size_t actual_len = min_sz(len, free_space);

    if (actual_len == 0)
        return 0;

    /* First chunk: from head to end of buffer */
    size_t first_chunk = min_sz(actual_len, rb->capacity - rb->head);
    memcpy(rb->buf + rb->head, data, first_chunk);

    /* Second chunk wraps around */
    if (actual_len > first_chunk) {
        memcpy(rb->buf, data + first_chunk, actual_len - first_chunk);
        rb->head = actual_len - first_chunk;
    } else {
        rb->head += first_chunk;
        rb->head %= rb->capacity;
    }

    rb->count += actual_len;
    return actual_len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t actual_len = min_sz(len, rb->count);

    /* First chunk: from tail to end of buffer */
    size_t first_chunk = min_sz(actual_len, rb->capacity - rb->tail);
    memcpy(out, rb->buf + rb->tail, first_chunk);

    /* Second chunk wraps around */
    if (actual_len > first_chunk) {
        memcpy(out + first_chunk, rb->buf, actual_len - first_chunk);
        rb->tail = actual_len - first_chunk;
    } else {
        rb->tail += first_chunk;
        rb->tail %= rb->capacity;
    }

    rb->count -= actual_len;
    return actual_len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->count == 0)
        return 0;

    size_t actual_len = min_sz(len, rb->count);

    size_t tmp_tail = rb->tail;
    size_t first_chunk = min_sz(actual_len, rb->capacity - tmp_tail);
    memcpy(out, rb->buf + tmp_tail, first_chunk);

    if (actual_len > first_chunk) {
        memcpy(out + first_chunk, rb->buf, actual_len - first_chunk);
    }

    return actual_len;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
