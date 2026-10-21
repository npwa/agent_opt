#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;       /* index of the oldest byte (read pointer) */
    size_t tail;       /* index of the next write position      */
    size_t data_len;   /* bytes currently stored                  */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb = malloc(sizeof(*rb));
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
    rb->data_len = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t n, to_end, remain;

    if (!rb || !data || len == 0)
        return 0;

    if (rb->data_len == rb->capacity)
        return 0;              /* full */

    n = len < (rb->capacity - rb->data_len) ? len : (rb->capacity - rb->data_len);

    to_end = rb->capacity - rb->tail;   /* space until end of buffer */
    if (n <= to_end) {
        memcpy(rb->buf + rb->tail, data, n);
    } else {
        remain = n - to_end;
        memcpy(rb->buf + rb->tail, data, to_end);
        memcpy(rb->buf, data + to_end, remain);
    }

    rb->tail = (rb->tail + n) % rb->capacity;
    rb->data_len += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n, to_end;

    if (!rb || !out || len == 0 || rb->data_len == 0)
        return 0;

    n = len < rb->data_len ? len : rb->data_len;

    to_end = rb->capacity - rb->head;   /* space until end of buffer */
    if (n <= to_end) {
        memcpy(out, rb->buf + rb->head, n);
    } else {
        memcpy(out, rb->buf + rb->head, to_end);
        memcpy(out + to_end, rb->buf, n - to_end);
    }

    rb->head = (rb->head + n) % rb->capacity;
    rb->data_len -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t n, to_end;

    if (!rb || !out || len == 0 || rb->data_len == 0)
        return 0;

    n = len < rb->data_len ? len : rb->data_len;

    to_end = rb->capacity - rb->head;
    if (n <= to_end) {
        memcpy(out, rb->buf + rb->head, n);
    } else {
        memcpy(out, rb->buf + rb->head, to_end);
        memcpy(out + to_end, rb->buf, n - to_end);
    }

    return n;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->data_len;
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
    rb->data_len = 0;
}
