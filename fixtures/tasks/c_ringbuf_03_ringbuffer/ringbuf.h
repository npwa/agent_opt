#ifndef RINGBUF_H
#define RINGBUF_H

#include <stddef.h>
#include <stdint.h>

/* Fixed-capacity FIFO ring buffer of bytes. Not thread-safe. */
typedef struct ringbuf ringbuf_t;

/* Create a buffer holding up to `capacity` bytes (capacity > 0).
 * Returns NULL on invalid capacity or allocation failure. */
ringbuf_t *rb_create(size_t capacity);

/* Free the buffer. Accepts NULL. */
void rb_destroy(ringbuf_t *rb);

/* Append up to `len` bytes from `data`. Writes as many bytes as fit and
 * returns the number actually written (0 if full or arguments are NULL). */
size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len);

/* Remove up to `len` bytes into `out`, oldest first. Returns the number
 * actually read (0 if empty or arguments are NULL). */
size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len);

/* Copy up to `len` oldest bytes into `out` without removing them. */
size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len);

/* Number of bytes currently stored. */
size_t rb_size(const ringbuf_t *rb);

/* Total capacity given at creation. */
size_t rb_capacity(const ringbuf_t *rb);

/* Discard all content. */
void rb_clear(ringbuf_t *rb);

#endif
