#include "ringbuf.h"

#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t *buf;
    size_t capacity;
    size_t head;       /* next write position */
    size_t tail;       /* oldest data position */
    size_t count;      /* bytes currently stored */
};

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    ringbuf_t *rb = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

static size_t write_avail(const ringbuf_t *rb)
{
    return rb->capacity - rb->count;
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0)
        return 0;

    size_t avail = write_avail(rb);
    if (avail == 0)
        return 0;

    size_t to_write = len < avail ? len : avail;

    /* Write in two possible segments (wrap-around) */
    size_t first  = rb->capacity - rb->head;          /* space from head to end */
    size_t part1 = first < to_write ? first : to_write;

    memcpy(rb->buf + rb->head, data, part1);
    rb->head = (rb->head + part1) % rb->capacity;

    if (part1 < to_write) {
        size_t part2 = to_write - part1;
        memcpy(rb->buf, data + part1, part2);
        rb->head = part2 % rb->capacity;              /* head is now at part2 */
    }

    rb->count += to_write;
    return to_write;
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0 || len == 0)
        return 0;

    size_t to_read = rb->count < len ? rb->count : len;

    /* Read in two possible segments (wrap-around) */
    size_t first  = rb->capacity - rb->tail;          /* space from tail to end */
    size_t part1 = first < to_read ? first : to_read;

    memcpy(out, rb->buf + rb->tail, part1);
    rb->tail = (rb->tail + part1) % rb->capacity;

    if (part1 < to_read) {
        size_t part2 = to_read - part1;
        memcpy(out + part1, rb->buf, part2);
        rb->tail = part2 % rb->capacity;              /* tail is now at part2 */
    }

    rb->count -= to_read;
    return to_read;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || rb->count == 0 || len == 0)
        return 0;

    size_t to_copy = rb->count < len ? rb->count : len;
    size_t tmp_tail = rb->tail;

    size_t first  = rb->capacity - tmp_tail;
    size_t part1 = first < to_copy ? first : to_copy;

    memcpy(out, rb->buf + tmp_tail, part1);
    tmp_tail = (tmp_tail + part1) % rb->capacity;

    if (part1 < to_copy) {
        size_t part2 = to_copy - part1;
        memcpy(out + part1, rb->buf, part2);
    }

    return to_copy;
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
