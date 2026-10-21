#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

/* ---- internal struct (opaque to callers) ---- */
struct ringbuf {
    uint8_t *data;       /* storage, allocated with calloc          */
    size_t   capacity;   /* nominal max byte count (rb_capacity())  */
    size_t   head;       /* index of oldest byte (first read pos)   */
    size_t   tail;       /* index where next write goes             */
    size_t   cnt;        /* bytes currently held in the ring        */
};

/* ---- public API ----------------------------------------------------- */

ringbuf_t *rb_create(size_t capacity)
{
    if (capacity == 0) return NULL;

    ringbuf_t *rb = (ringbuf_t *)calloc(1, sizeof *rb);
    if (!rb) return NULL;

    rb->data = (uint8_t *)calloc(capacity + 1u, sizeof(uint8_t));   /* guard byte */
    if (!rb->data) { free(rb); return NULL; }

    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->cnt      = 0;
    return rb;
}

void rb_destroy(ringbuf_t *rb)          /* tolerates NULL */
{
    if (!rb) return;
    free(rb->data);
    free(rb);
}

/* --- internal helper -------------------------------------------------- */
static inline size_t write_avail(const ringbuf_t *rb) { return rb->capacity - rb->cnt; }

/* append up to *len* bytes from *data*.  Returns bytes actually stored. */
size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0) return 0;

    size_t avail = write_avail(rb);
    if (avail == 0)              return 0;

    size_t n   = (len < avail) ? len : avail;      /* clamp at free space */
    size_t wrap_pt = rb->capacity - rb->tail;          /* tail → end-of-array */

    if (n <= wrap_pt) {
        /* fits in the contiguous tail segment — no wrap needed */
        memcpy(rb->data + rb->tail, data, n);
    } else {
        /* spans the array boundary:
           chunk 1: fill from tail to end       */
        memcpy(rb->data + rb->tail, data, wrap_pt);

        /* chunk 2: remaining bytes at start */
        size_t rest = n - wrap_pt;
        memcpy(rb->data, data + wrap_pt, rest);
    }

    rb->tail = (rb->tail + n) % rb->capacity;
    rb->cnt += n;
    return n;
}

/* remove up to *len* oldest bytes into *out*.  Returns bytes actually read. */
size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->cnt == 0) return 0;

    size_t n   = (len < rb->cnt) ? len : rb->cnt;  /* clamp at available */
    size_t avail = rb->capacity - rb->head;          /* head → end-of-array */

    if (n <= avail) {
        memcpy(out, rb->data + rb->head, n);
    } else {
        /* spans the boundary */
        memcpy(out, rb->data + rb->head, avail);
        size_t rest = n - avail;
        memcpy(out + avail, rb->data, rest);
    }

    rb->head = (rb->head + n) % rb->capacity;
    rb->cnt -= n;
    return n;
}

/* copy up to *len* oldest bytes into *out* WITHOUT removing them.       */
size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
    if (!rb || !out || len == 0 || rb->cnt == 0) return 0;

    size_t n   = (len < rb->cnt) ? len : rb->cnt;
    size_t avail = rb->capacity - rb->head;

    if (n <= avail) {
        memcpy(out, rb->data + rb->head, n);
    } else {
        memcpy(out, rb->data + rb->head, avail);
        size_t rest = n - avail;
        memcpy(out + avail, rb->data, rest);
    }

    /* head / tail / cnt NOT modified */
    return n;
}

size_t rb_size(const ringbuf_t *rb) { return rb ? rb->cnt : 0; }
size_t rb_capacity(const ringbuf_t *rb) { return rb ? rb->capacity : 0; }

void rb_clear(ringbuf_t *rb)          /* drop all content               */
{
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->cnt  = 0;
}


/* ---- smoke tests (self-contained, no harness needed) ----------------- */
#include <stdio.h>

#define CHECK(cond, msg)                                               \
    do {                                                               \
        if (!(cond)) { fprintf(stderr, "FAIL %s at line %d\n", msg, __LINE__); return 1; } \
    } while(0)

int main(void)
{
    printf("[ ringbuf smoke tests ]\n");

    /* 1. create / capacity / destroy-NULL -------------------------- */
    {
        ringbuf_t *rb = rb_create(16); CHECK(rb != NULL, "create");
        CHECK(rb_capacity(rb) == 16,   "capacity");
        CHECK(rb_size(rb)     == 0,    "initial size");

        rb_destroy(NULL);
        rb_destroy(rb);
    }

    /* 2. invalid capacity ------------------------------------------ */
    { ringbuf_t *rb = rb_create(0); CHECK(rb == NULL, "zero-capacity"); }

    /* 3. basic write / read round-trip ----------------------------- */
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t msg[] = {'a','b'};
        CHECK(rb_write(rb, msg, 2) == 2, "write");
        CHECK(rb_size(rb)             == 2,     "size-after-write");

        uint8_t out[16]; memset(out, 0xAA, sizeof(out));
        CHECK(rb_read(rb, out, sizeof(out)) ==  2, "read");
        CHECK(out[0] == (uint8_t)'a' && out[1] == (uint8_t)'b',     "data");
        rb_destroy(rb);
    }

    /* 4. wrap-around write ----------------------------------------- */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t a[] = {'1','2','3'};   /* fills 3 / 4 slots */
        CHECK(rb_write(rb, a, sizeof(a)) ==                  3, "w1");

        /* drain one at head → frees 1 slot, ring wraps on next write */
        { uint8_t dummy; rb_read(rb, &dummy, 1); }
        CHECK(rb_size(rb) == 2, "sz after r1");
        uint8_t b[] = {'X','Y','Z','W'};   /* write 4 into ring of cap-4 */
        CHECK(rb_write(rb, b, sizeof(b)) ==                  2, "w2-fit wraps");

        rb_destroy(rb);
    }

    /* 5. partial-write (data larger than ring) --------------------- */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t big[] = {'a','b','c','d','e'};
        CHECK(rb_write(rb, big, sizeof(big))   ==                  4, "partial-w");

        uint8_t out[16]; memset(out, 0xBB, sizeof(out));
        CHECK(rb_read(rb, out, sizeof(out))    ==                  4, "read-all");
        CHECK(memcmp(out, big, 4) ==                               0, "data-correct");
        rb_destroy(rb);
    }

    /* 6. clear / re-use ------------------------------------------------ */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t d[] = {'?'};   CHECK(rb_write(rb, d,1) ==1,"w");
        rb_clear(rb);
        CHECK(rb_size(rb)               ==              0 ,     "cleared");
        CHECK(rb_write(rb, d, sizeof(d)) == 1,                "re-write");
        rb_destroy(rb);
    }

    printf("All tests passed.\n");
    return 0;
}
