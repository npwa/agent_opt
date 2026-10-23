#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;   /* write index */
    size_t tail;   /* read index */
    size_t count;  /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->buf = malloc(capacity);
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

static size_t free_space(const ringbuf_t *rb)
{
    return rb->capacity - rb->count;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = free_space(rb);
    if (avail == 0)
        return 0;

    size_t to_write = len < avail ? len : avail;

    /* First segment: from head to end of buffer */
    size_t first = rb->capacity - rb->head;
    size_t seg1 = to_write < first ? to_write : first;
    memcpy(rb->buf + rb->head, data, seg1);
    rb->head += seg1;
    if (rb->head == rb->capacity)
        rb->head = 0;

    /* Second segment: wraps around */
    size_t remaining = to_write - seg1;
    if (remaining > 0) {
        memcpy(rb->buf + rb->head, data + seg1, remaining);
        rb->head += remaining;
    }

    rb->count += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    size_t to_read = len < rb->count ? len : rb->count;

    /* First segment: from tail to end of buffer */
    size_t first = rb->capacity - rb->tail;
    size_t seg1 = to_read < first ? to_read : first;
    memcpy(out, rb->buf + rb->tail, seg1);
    rb->tail += seg1;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    /* Second segment: wraps around */
    size_t remaining = to_read - seg1;
    if (remaining > 0) {
        memcpy(out + seg1, rb->buf + rb->tail, remaining);
        rb->tail += remaining;
    }

    rb->count -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->count == 0)
        return 0;

    size_t to_copy = len < rb->count ? len : rb->count;
    /* Use a temp index so we don't mutate state */
    size_t tmp_tail = rb->tail;
    size_t to_read = to_copy;

    size_t first = rb->capacity - tmp_tail;
    size_t seg1 = to_read < first ? to_read : first;
    memcpy(out, rb->buf + tmp_tail, seg1);
    tmp_tail += seg1;
    if (tmp_tail == rb->capacity)
        tmp_tail = 0;

    size_t remaining = to_read - seg1;
    if (remaining > 0) {
        memcpy(out + seg1, rb->buf + tmp_tail, remaining);
        /* no need to advance tmp_tail further */
    }

    return to_copy;
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
