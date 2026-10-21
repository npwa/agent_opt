#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;
    if (capacity == 0) return NULL;
    rb = malloc(sizeof(*rb));
    if (!rb) return NULL;
    rb->storage = malloc(capacity);
    if (!rb->storage) { free(rb); return NULL; }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb) return;
    free(rb->storage);
    free(rb);
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t space, amt, head_end;
    if (!rb || !data || len == 0) return 0;
    space = rb->capacity - rb->count;
    if (space == 0) return 0;
    amt = len < space ? len : space;
    head_end = rb->capacity - rb->head;
    if (amt <= head_end) {
        memcpy(rb->storage + rb->head, data, amt);
    } else {
        memcpy(rb->storage + rb->head, data, head_end);
        memcpy(rb->storage, data + head_end, amt - head_end);
    }
    rb->head = (rb->head + amt) % rb->capacity;
    rb->count += amt;
    return amt;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t avail, cnt1, tail_end;
    if (!rb || !out || len == 0 || rb->count == 0) return 0;
    avail = rb->count < len ? rb->count : len;
    tail_end = rb->capacity - rb->tail;
    if (avail <= tail_end) {
        memcpy(out, rb->storage + rb->tail, avail);
    } else {
        cnt1 = tail_end;
        memcpy(out,           rb->storage + rb->tail, cnt1);
        memcpy(out + cnt1,    rb->storage,            avail - cnt1);
    }
    rb->tail = (rb->tail + avail) % rb->capacity;
    rb->count -= avail;
    return avail;
}

size_t rb_peek(const ringbuf_t *rb_in, uint8_t *out, size_t len)
{
    const struct ringbuf *rb = (const struct ringbuf *)rb_in;
    size_t avail, cnt1, tail_end;
    if (!rb || !out || len == 0 || rb->count == 0) return 0;
    avail = rb->count < len ? rb->count : len;
    tail_end = rb->capacity - rb->tail;
    if (avail <= tail_end) {
        memcpy(out, rb->storage + rb->tail, avail);
    } else {
        cnt1 = tail_end;
        memcpy(out,           rb->storage + rb->tail, cnt1);
        memcpy(out + cnt1,    rb->storage,            avail - cnt1);
    }
    return avail;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
