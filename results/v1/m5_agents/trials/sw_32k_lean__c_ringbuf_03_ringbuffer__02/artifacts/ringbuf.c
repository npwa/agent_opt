#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *storage;  /* internal circular buffer                   */
    size_t   head;     /* next write index (== end of stored data)  */
    size_t   tail;     /* next read index  (== start of stored data)*/
    size_t   size;     /* bytes currently inside the ring            */
    size_t   capacity; /* total storage capacity from creation        */
};

ringbuf_t *rb_create(size_t cap)
{
    if (!cap) return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;

    rb->storage = calloc(cap, sizeof(uint8_t));
    if (!rb->storage) { free(rb); return NULL; }
    rb->head     = 0;
    rb->tail     = 0;
    rb->size     = 0;
    rb->capacity = cap;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb) return;
    free(rb->storage);
    free(rb);
}

/* Copy `cnt` bytes from `src` into ringbuf starting at head.
   Caller guarantees all cnt bytes fit in the storage (no bounds
   checked here).  Data may straddle the physical boundary.     */
static void wb_store(ringbuf_t *rb, const uint8_t *src, size_t cnt)
{
    if (cnt == 0) return;

    /* How many bytes can we write before hitting the end of array? */
    size_t n1 = rb->capacity - rb->head;

    if (n1 >= cnt) {
        /* All fits without wrapping */
        memcpy(rb->storage + rb->head, src, cnt);
    } else {
        /* Must wrap: first chunk to end, second from start */
        memcpy(rb->storage + rb->head, src, n1);
        memcpy(rb->storage, src + n1, cnt - n1);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data) return 0;
    if (len == 0) return 0;

    /* Available free space in the ring */
    size_t free = rb->capacity - rb->size;
    size_t wrote = len > free ? free : len;
    if (wrote == 0) return 0;

    wb_store(rb, data, wrote);

    rb->head     = (rb->head + wrote) % rb->capacity;
    rb->size    += wrote;
    return wrote;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out) return 0;
    if (len == 0) return 0;
    if (!rb->size) return 0;

    size_t taken = rb->size < len ? rb->size : len;

    /* Copy from tail position to out[], wrapping storage index as needed */
    for (size_t i = 0; i < taken; ++i) {
        out[i] = rb->storage[(rb->tail + i) % rb->capacity];
    }

    rb->tail     = (rb->tail + taken) % rb->capacity;
    rb->size    -= taken;
    return taken;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out) return 0;
    if (len == 0) return 0;
    if (!rb->size) return 0;

    size_t avail = rb->size < len ? rb->size : len;

    for (size_t i = 0; i < avail; ++i) {
        out[i] = rb->storage[(rb->tail + i) % rb->capacity];
    }
    return avail;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->size;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->capacity;
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb) return;
    rb->head   = 0;
    rb->tail   = 0;
    rb->size   = 0;
}
