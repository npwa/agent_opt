#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;     /* max bytes (storage length) */
    size_t size;         /* currently stored bytes      */
    size_t head;         /* next write index           */
    size_t tail;         /* next read  index           */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->storage = calloc(capacity, sizeof(uint8_t));
    if (!rb->storage) {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = rb->capacity - rb->size;
    if (avail == 0)
        return 0;

    size_t n = len < avail ? len : avail;

    /* Try to fit in one contiguous chunk from head */
    size_t first_chunk = rb->capacity - rb->head; /* bytes from head to end */
    size_t copy1 = n < first_chunk ? n : first_chunk;

    memcpy(rb->storage + rb->head, data, copy1);
    rb->head += copy1;
    if (rb->head >= rb->capacity)
        rb->head -= rb->capacity;

    /* Wrap around */
    size_t copy2 = n - copy1;
    if (copy2 > 0) {
        memcpy(rb->storage + rb->head, data + copy1, copy2);
        rb->head += copy2;
    }

    rb->size += n;
    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t n = len < rb->size ? len : rb->size;

    size_t first_chunk = rb->capacity - rb->tail;
    size_t copy1 = n < first_chunk ? n : first_chunk;

    memcpy(out, rb->storage + rb->tail, copy1);
    rb->tail += copy1;
    if (rb->tail >= rb->capacity)
        rb->tail -= rb->capacity;

    size_t copy2 = n - copy1;
    if (copy2 > 0) {
        memcpy(out + copy1, rb->storage + rb->tail, copy2);
        rb->tail += copy2;
    }

    rb->size -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->size == 0)
        return 0;

    size_t n = len < rb->size ? len : rb->size;
    size_t pos = rb->tail; /* don't advance tail */

    size_t first_chunk = rb->capacity - pos;
    size_t copy1 = n < first_chunk ? n : first_chunk;

    memcpy(out, rb->storage + pos, copy1);

    size_t copy2 = n - copy1;
    if (copy2 > 0) {
        memcpy(out + copy1, rb->storage, copy2);
    }

    return n;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->size;
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
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
}
