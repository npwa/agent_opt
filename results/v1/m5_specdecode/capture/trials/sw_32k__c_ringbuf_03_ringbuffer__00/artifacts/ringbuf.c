#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf {
    uint8_t *store;
    size_t capacity;
    size_t head;  /* write index */
    size_t tail;  /* read index */
    size_t count; /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (!rb)
        return NULL;

    rb->store = calloc(capacity, sizeof(uint8_t));
    if (!rb->store) {
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
    free(rb->store);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    if (rb->count == rb->capacity)
        return 0;

    /* How many slots are free? */
    size_t free_space = rb->capacity - rb->count;
    size_t to_write = len < free_space ? len : free_space;

    /* First segment: write from head to end of buffer (or until tail) */
    size_t first_chunk = to_write;
    size_t tail_wrap = rb->capacity - rb->head;
    if (first_chunk > tail_wrap)
        first_chunk = tail_wrap;

    /* If head is ahead of tail and we can wrap, make sure we don't overwrite tail.
     * When count < capacity there's always room; the segments below handle it correctly. */

    /* Copy to end of buffer if needed */
    {
        size_t n = first_chunk;
        while (n--)
            rb->store[rb->head++] = *data++;
        rb->head %= rb->capacity;
    }

    /* Second segment if write wraps around */
    if (to_write > first_chunk) {
        size_t second_chunk = to_write - first_chunk;
        /* tail must be ahead of where we'll write, because count < capacity
         * so there's free space after head before reaching tail. */
        while (second_chunk--)
            rb->store[rb->head++] = *data++;
        rb->head %= rb->capacity;
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
    {
        size_t i;
        for (i = 0; i < to_read; i++) {
            *out++ = rb->store[rb->tail];
            rb->tail = (rb->tail + 1) % rb->capacity;
        }
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

    size_t to_read = len < rb->count ? len : rb->count;
    size_t tmp_tail = rb->tail;
    size_t i;

    for (i = 0; i < to_read; i++) {
        *out++ = rb->store[tmp_tail];
        tmp_tail = (tmp_tail + 1) % rb->capacity;
    }

    return to_read;
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
