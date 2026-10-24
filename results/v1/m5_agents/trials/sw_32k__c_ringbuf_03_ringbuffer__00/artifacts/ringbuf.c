#include "ringbuf.h"
#include <stdlib.h>

struct ringbuf_t {
    uint32_t *data;
    size_t head;   /* index of oldest element */
    size_t tail;   /* next free slot for push */
    size_t count;  /* current number of elements */
    size_t capacity;
};

ringbuf_t *ringbuf_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->data = calloc(capacity, sizeof(uint32_t));
    if (!rb->data) {
        free(rb);
        return NULL;
    }

    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
    rb->capacity = capacity;
    return rb;
}

void ringbuf_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->data);
    free(rb);
}

int ringbuf_push(ringbuf_t *rb, uint32_t item)
{
    if (!rb)
        return 0;

    /* If full, overwrite oldest: advance both head and tail */
    if (rb->count == rb->capacity) {
        rb->data[rb->tail] = item;
        rb->head = (rb->head + 1) % rb->capacity;
        rb->tail = (rb->tail + 1) % rb->capacity;
    } else {
        rb->data[rb->tail] = item;
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count++;
    }
    return 1;
}

int ringbuf_pop(ringbuf_t *rb, uint32_t *out_item)
{
    if (!rb || rb->count == 0)
        return 0;

    if (out_item)
        *out_item = rb->data[rb->head];

    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    return 1;
}

int ringbuf_peek(const ringbuf_t *rb, int index, uint32_t *out_item)
{
    if (!rb || index < 0 || (size_t)index >= rb->count)
        return 0;

    int real_idx = (rb->head + index) % rb->capacity;
    if (out_item)
        *out_item = rb->data[real_idx];
    return 1;
}

size_t ringbuf_count(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->count;
}

int ringbuf_is_full(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return (int)(rb->count == rb->capacity);
}

int ringbuf_is_empty(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return (int)(rb->count == 0);
}

size_t ringbuf_capacity(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->capacity;
}
