#include "ringbuf.h"
#include <string.h>
#include <stdlib.h>

struct ringbuf {
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
    uint8_t *buf;
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    uint8_t *p = calloc(capacity, sizeof(uint8_t));
    if (!p) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size     = 0;
    rb->head     = 0;
    rb->tail     = 0;
    rb->buf      = p;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

size_t rb_size(const ringbuf_t *rb)
{
    return rb ? rb->size : 0;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    return rb ? rb->capacity : 0;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->size     = 0;
    rb->head     = 0;
    rb->tail     = 0;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    /* How many slots are free? */
    if (rb->size >= rb->capacity)
        return 0;

    size_t avail = rb->capacity - rb->size;
    size_t n     = (len < avail) ? len : avail;

    /* Write as two linear copies if tail would cross the boundary */
    {
        size_t c1, c2 = 0;
        size_t room_at_tail = rb->capacity - rb->tail;

        if (room_at_tail >= n) {
            c1 = n;
        } else {
            c1 = room_at_tail;
            c2 = n - c1;
        }
        if (data) {
            memcpy(rb->buf + rb->tail, data, c1);
        }
        if (c2 > 0 && data) {
            memcpy(rb->buf, data + c1, c2);
        }
    }

    /* Advance tail */
    rb->head = rb->tail;   /* we need this! ... no, we don't track head in write */
    rb->tail = (rb->tail + n) % rb->capacity;
    rb->size += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t n = (len < rb->size) ? len : rb->size;

    /* Read as two linear copies if head would cross the boundary */
    {
        size_t c1, c2 = 0;
        size_t room_at_head = rb->capacity - rb->head;

        if (room_at_head >= n) {
            c1 = n;
        } else {
            c1 = room_at_head;
            c2 = n - c1;
        }
        if (out) {
            memcpy(out, rb->buf + rb->head, c1);
        }
        if (c2 > 0 && out) {
            memcpy(out + c1, rb->buf, c2);
        }
    }

    rb->size -= n;
    rb->head = (rb->head + n) % rb->capacity;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t n = (len < rb->size) ? len : rb->size;

    /* Copy from current head position, advancing through the ring */
    {
        size_t idx   = rb->head;
        size_t c1    = rb->capacity - rb->head;
        size_t copy_n = (n <= c1) ? n : c1;

        if (out) {
            memcpy(out, rb->buf + idx, copy_n);
        }
        /* Advance local idx */
        idx += copy_n;

        if (n > copy_n) {
            size_t remaining = n - copy_n;
            if (remaining < rb->capacity - (idx % rb->capacity)) {
                /* fits in one more linear copy at the beginning of the buffer */
                memcpy(out + copy_n, rb->buf, remaining);
                return n;
            } else {
                /* must split this part too */
                size_t c2 = rb->capacity - (idx % rb->capacity);
                if (remaining >= c2) {
                    memcpy(out + copy_n, rb->buf, c2);
                    memcpy(out + copy_n + c2, rb->buf, remaining - c2);
                    return n;
                } else {
                    memcpy(out + copy_n, rb->buf + (idx % rb->capacity), remaining);
                    return n;
                }
            }
        }
    }
    return n;
}
