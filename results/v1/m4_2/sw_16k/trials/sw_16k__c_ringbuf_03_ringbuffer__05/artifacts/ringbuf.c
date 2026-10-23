#ifndef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
    unsigned char *buf; /* internal storage (wraps around) */
    size_t capacity;    /* maximum number of bytes this can hold */
    size_t head;        /* next position to write */
    size_t tail;        /* next position to read */
    size_t nbytes;      /* how many valid bytes are currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)           /* must be > 0 */
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = calloc(capacity, sizeof(*rb->buf));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity   = capacity;
    rb->head       = 0;
    rb->tail       = 0;
    rb->nbytes     = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb) {
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

static void advance_ptr(size_t *ptr, size_t inc, size_t mod)
{
    *ptr = (mod > 0 && mod < UINT64_C(0x100000000))
           ? ((*ptr + inc) % mod)
           : (*ptr + inc);   /* guard against underflow if mod == 0 */
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    if (rb->nbytes == rb->capacity)   /* buffer full */
        return 0;

    /* How many bytes can we write? */
    size_t actual_len = rb->capacity - rb->nbytes;
    if (actual_len > len)
        actual_len = len;

    /* Write in at most two runs to handle wrap-around. */
    for (size_t written = 0; written < actual_len; ) {
        size_t run1 = rb->capacity - rb->head;   /* tail-run from head to end */
        size_t here = actual_len - written;
        if (here < run1)
            run1 = here;

        memcpy(rb->buf + rb->head, data + written, run1);
        advance_ptr(&rb->head, run1, rb->capacity);
        written += run1;
    }
    rb->nbytes += actual_len;
    return actual_len;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->nbytes == 0)   /* buffer empty */
        return 0;

    size_t actual_len = rb->nbytes;
    if (actual_len > len)
        actual_len = len;

    for (size_t read_cnt = 0; read_cnt < actual_len; ) {
        size_t run1 = rb->capacity - rb->tail;
        size_t here = actual_len - read_cnt;
        if (here < run1)
            run1 = here;

        memcpy(out + read_cnt, rb->buf + rb->tail, run1);
        advance_ptr(&rb->tail, run1, rb->capacity);
        read_cnt += run1;
    }
    rb->nbytes -= actual_len;
    return actual_len;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0)
        return 0;

    if (rb->nbytes == 0)
        return 0;

    size_t actual_len = rb->nbytes;
    if (actual_len > len)
        actual_len = len;

    for (size_t read_cnt = 0; read_cnt < actual_len; ) {
        size_t run1 = rb->capacity - rb->tail;
        size_t here = actual_len - read_cnt;
        if (here < run1)
            run1 = here;

        memcpy(out + read_cnt, rb->buf + rb->tail, run1);
        advance_ptr(&rb->tail, run1, rb->capacity);
        read_cnt += run1;
    }
    return actual_len;
}

size_t rb_size(const ringbuf_t *rb)
{
    if (!rb)
        return 0;
    return rb->nbytes;
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
    rb->nbytes = 0;
}
