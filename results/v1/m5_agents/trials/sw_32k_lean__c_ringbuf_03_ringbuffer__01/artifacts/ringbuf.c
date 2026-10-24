#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    size_t capacity;   /* total slots (capacity > 0)   */
    size_t size;       /* bytes currently stored        */
    size_t head;       /* next write index              */
    size_t tail;       /* next read index               */
    uint8_t *storage;  /* the backing buffer            */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->storage = calloc(capacity, sizeof(uint8_t));   /* zeroed storage */
    if (!rb->storage) {
        free(rb);
        return NULL;
    }

    rb->capacity  = capacity;
    rb->size      = 0;
    rb->head      = 0;
    rb->tail      = 0;
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

    /* Space available = capacity - filled */
    size_t avail = rb->capacity - rb->size;
    if (avail == 0)
        return 0;

    size_t to_write = len < avail ? len : avail;   /* at most what fits */
    size_t until_end = rb->capacity - rb->head;      /* bytes to end of buffer */

    if (to_write <= until_end) {
        /* All fit before wrapping */
        memcpy(rb->storage + rb->head, data, to_write);
        rb->head += to_write;
    } else {
        /* Copy in two chunks: head...end, then start...tail-1  */
        memcpy(rb->storage + rb->head, data, until_end);
        memcpy(rb->storage, data + until_end, to_write - until_end);
        rb->head = to_write - until_end;
    }
    rb->size += to_write;
    return to_write;
}

static size_t read_chunk(ringbuf_t *rb, uint8_t *out, size_t offset, size_t n)
{
    if (n == 0 || offset >= rb->capacity)
        return 0;

    size_t until_end = rb->capacity - offset;
    size_t first_part = until_end < n ? until_end : n;

    memcpy(out, rb->storage + offset, first_part);
    out += first_part;

    size_t second = n - first_part;
    if (second > 0)
        memcpy(out, rb->storage, second);

    return n;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->size;
    if (avail == 0)
        return 0;

    size_t to_read = len < avail ? len : avail;   /* at most what is stored */
    read_chunk(rb, out, rb->tail, to_read);

    rb->tail += to_read;
    rb->size -= to_read;
    if (rb->tail == rb->capacity)       /* wrap back */
        rb->tail = 0;                    /* capacity > 0 here */
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    size_t avail = rb->size;
    if (avail == 0)
        return 0;

    size_t to_copy = len < avail ? len : avail;
    read_chunk((ringbuf_t *)rb, out, rb->tail, to_copy);   /* cast OK: we don't modify */
    return to_copy;
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
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
