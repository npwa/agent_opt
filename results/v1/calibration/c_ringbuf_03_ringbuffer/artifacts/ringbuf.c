#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

/* Ring buffer that reserves one spare slot so that count == capacity means "full", not ambiguous with head == tail. */
#define SPARE_SLOT_SIZE 1

struct ringbuf {
    uint8_t *buf;          /* raw storage, allocated to capacity bytes */
    size_t   cap;          /* total slots in buf (effective capacity is cap - 1)   */
    size_t   head;         /* index of next write position                     */
    size_t   tail;         /* index of next read  position                     */
    size_t   count;        /* number of bytes currently stored                 */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0) return NULL;          /* invalid capacity     */

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb) return NULL;

    rb->cap = capacity + SPARE_SLOT_SIZE;    /* add one slot for full/empty distinction   */
    rb->buf = calloc(rb->cap, sizeof(uint8_t));
    if (!rb->buf) { free(rb); return NULL; }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb) return;                           /* per spec: NULL is accepted   */
    free(rb->buf);
    free(rb);
}

/* ------------------------------------------------------------------ */

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0) return 0;

    /* How many bytes can we still store? */
    size_t avail = rb->cap - 1 - rb->count;  /* cap-1 = effective capacity   */
    if (len > avail) len = avail;
    if (len == 0) return 0;

    /* Write first chunk up to the end of the buffer                    */
    size_t first = rb->cap - rb->head;
    if (first > len) first = len;

    memcpy(rb->buf + rb->head, data, first);
    rb->head = (rb->head + first) % rb->cap;
    rb->count += first;
    data      += first;

    /* Write second chunk that wraps to the beginning (if any)          */
    if (len > first) {
        size_t rest = len - first;                /* should be <= head now       */
        memcpy(rb->buf, data, rest);
        rb->head = rest % rb->cap;              /* same as `rest` since rest < cap   */
        rb->count += rest;
    }

    return len;            /* the count we actually wrote (== original len after capping)  */
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0) return 0;

    if (len > rb->count) len = rb->count;   /* clamp to what's available   */
    if (len == 0) return 0;

    /* First chunk from tail up to end                                  */
    size_t first = rb->cap - rb->tail;
    if (first > len) first = len;

    memcpy(out, rb->buf + rb->tail, first);
    rb->tail = (rb->tail + first) % rb->cap;
    rb->count -= first;
    out       += first;

    /* Second chunk that wraps to the beginning                         */
    if (len > first) {
        size_t rest = len - first;
        memcpy(out, rb->buf, rest);
        rb->tail = rest % rb->cap;
        rb->count -= rest;
    }

    return len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0) return 0;

    if (len > rb->count) len = rb->count;   /* clamp to what's available   */
    if (len == 0) return 0;

    /* Same two-chunk copy as read -- but we only advance 'tail' locally  */
    size_t first = rb->cap - rb->tail;
    if (first > len) first = len;

    memcpy(out, rb->buf + rb->tail, first);
    out += first;
    size_t pos = rb->tail + first;         /* advance head of our read window   */

    if (len > first) {
        size_t rest = len - first;
        memcpy(out, rb->buf + (pos % rb->cap), rest);
    }

    return len;                           /* don't mutate rb                  */
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->count;
}

size_t rb_capacity(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->cap - SPARE_SLOT_SIZE;       /* report the user-visible capacity */
}

void rb_clear(ringbuf_t *rb)
{
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
