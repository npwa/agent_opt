# Ring Buffer Specification

## Overview
A general-purpose fixed-size circular buffer (ring buffer) for C, using a dynamically allocated internal array.

## API

### Types
- `ringbuf_t` — opaque handle representing the ring buffer.

### Functions

#### `ringbuf_create(size_t capacity)`
Allocates and initializes a ring buffer with the given maximum number of elements.
Returns a pointer to the new ring buffer, or NULL on failure (capacity == 0 or OOM).

- On success: empty buffer; head == tail == 0; count == 0.

#### `ringbuf_destroy(ringbuf_t *rb)`
Frees all memory associated with the ring buffer. Safe if rb is NULL.

#### `ringbuf_push(ringbuf_t *rb, uint32_t item)`
Appends one element to the tail. If the buffer is full, discards the oldest element (head advances).
Returns 1 on success, 0 on failure (NULL rb).

- After push: count increases by 1 unless full (then count stays at capacity).

#### `ringbuf_pop(ringbuf_t *rb, uint32_t *out_item)`
Removes the head element and writes it into out_item if non-NULL.
Returns 1 on success, 0 on failure or if buffer was empty.

- After pop (success): count decreases by 1; tail unchanged.

#### `ringbuf_peek(ringbuf_t *rb, int index, uint32_t *out_item)`
Reads the element at position `index` (0 = oldest/head) without modifying state.
Returns 1 on success, 0 if rb is NULL or index >= count.

#### `ringbuf_count(ringbuf_t *rb)`
Returns the number of elements currently stored. Returns 0 if rb is NULL.

#### `ringbuf_is_full(ringbuf_t *rb)`
Returns 1 if the buffer is full, 0 otherwise. Returns 0 if rb is NULL.

#### `ringbuf_is_empty(ringbuf_t *rb)`
Returns 1 if the buffer is empty, 0 otherwise. Returns 0 if rb is NULL.

#### `ringbuf_capacity(ringbuf_t *rb)`
Returns the maximum number of elements the buffer can hold. Returns 0 if rb is NULL.

### Layout
Internal circular array uses modular arithmetic: `(tail + 1) % capacity` for advancing indices.

## Data type
Elements are fixed-size `uint32_t` (4 bytes).
