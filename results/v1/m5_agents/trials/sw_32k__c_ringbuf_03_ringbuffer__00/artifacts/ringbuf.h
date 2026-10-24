#ifndef RINGBUF_H
#define RINGBUF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuf_t ringbuf_t;

ringbuf_t *  ringbuf_create(size_t capacity);
void         ringbuf_destroy(ringbuf_t *rb);
int          ringbuf_push(ringbuf_t *rb, uint32_t item);
int          ringbuf_pop(ringbuf_t *rb, uint32_t *out_item);
int          ringbuf_peek(const ringbuf_t *rb, int index, uint32_t *out_item);
size_t       ringbuf_count(const ringbuf_t *rb);
int          ringbuf_is_full(const ringbuf_t *rb);
int          ringbuf_is_empty(const ringbuf_t *rb);
size_t       ringbuf_capacity(const ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUF_H */
