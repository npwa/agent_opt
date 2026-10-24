#include "ringbuf.h"

#include <stdlib.h>
#include <string.h>

struct ringbuf {
    uint8_t        *buf;   /* internal circular storage          */
    size_t          cap;   /* total capacity                     */
    size_t          used;  /* bytes currently stored             */
    size_t          wpos;  /* next write (tail) index            */
    size_t          rpos;  /* next read  (head) index            */
};

ringbuf_t *rb_create(size_t capacity)
{
    ringbuf_t *rb;

    if (capacity == 0)
        return NULL;

    rb       = malloc(sizeof(*rb));
    if (!rb)
        return NULL;

    rb->buf   = calloc(capacity, sizeof(uint8_t));
    if (!rb->buf) {
        free(rb);
        return NULL;
    }

    rb->cap    = capacity;
    rb->used   = 0;
    rb->wpos   = 0;
    rb->rpos   = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)
{
    if (!rb)
        return;
    free(rb->buf);
    free(rb);
}

/** Write `n` bytes starting at offset `src`.  The buffer may need two
 ** contiguous copies (tail→end-of-buf, then head→wpos).  Returns the
 ** total number of bytes placed. */
static void write_two_copy(uint8_t *buf, size_t cap, size_t wpos,
                           const uint8_t *src, size_t n)
{
    const size_t space_to_end = cap - wpos;

    if (n <= space_to_end) {
        memcpy(buf + wpos, src, n);
    } else {
        memcpy(buf + wpos, src, space_to_end);
        memcpy(buf, src + space_to_end, n - space_to_end);
    }
}

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data)
        return 0;

    if (len == 0 || rb->used == rb->cap)
        return 0;

    /* Clamp write size to available space */
    const size_t avail = rb->cap - rb->used;
    const size_t n     = len < avail ? len : avail;

    write_two_copy(rb->buf, rb->cap, rb->wpos, data, n);

    rb->wpos = (rb->wpos + n) % rb->cap;
    rb->used += n;
    return n;
}

/** Read `n` bytes starting at offset `dst`.  The buffer may need two
 ** contiguous copies (head→end-of-buf, then tail→rpos). */
static void read_two_copy(const uint8_t *buf, size_t cap, size_t rpos,
                          uint8_t *dst, size_t n)
{
    const size_t space_to_end = cap - rpos;

    if (n <= space_to_end) {
        memcpy(dst, buf + rpos, n);
    } else {
        memcpy(dst, buf + rpos, space_to_end);
        memcpy(dst + space_to_end, buf, n - space_to_end);
    }
}

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out)
        return 0;

    if (len == 0 || rb->used == 0)
        return 0;

    const size_t avail = rb->used;
    const size_t n     = len < avail ? len : avail;

    read_two_copy(rb->buf, rb->cap, rb->rpos, out, n);

    rb->rpos = (rb->rpos + n) % rb->cap;
    rb->used -= n;
    return n;
}

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out)
        return 0;

    if (len == 0 || rb->used == 0)
        return 0;

    const size_t avail = rb->used;
    const size_t n     = len < avail ? len : avail;

    read_two_copy(rb->buf, rb->cap, rb->rpos, out, n);
    /* Note: rpos and used are NOT advanced (peek is non-destructive) */
    return n;
}

size_t rb_size(const ringbuf_t *rb) { return rb ? rb->used : 0; }

size_t rb_capacity(const ringbuf_t *rb) { return rb ? rb->cap : 0; }

void rb_clear(ringbuf_t *rb)
{
    if (!rb)
        return;
    rb->wpos   = 0;
    rb->rpos   = 0;
    rb->used   = 0;
}
