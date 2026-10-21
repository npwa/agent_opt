#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

struct ringbuf {
	uint8_t *buf;
	size_t capacity;
	size_t head;	/* next write index */
	size_t tail;	/* next read  index */
	size_t count;	/* bytes currently stored */
};

/*------------------------------------------------------------------*/
/*  creation / destruction                                          */
/*------------------------------------------------------------------*/

ringbuf_t *rb_create(size_t capacity)
{
	ringbuf_t *rb;

	if (capacity == 0)
		return NULL;

	rb = malloc(sizeof(*rb));
	if (!rb)
		return NULL;

	rb->buf = calloc(capacity, sizeof(uint8_t));
	if (!rb->buf) {
		free(rb);
		return NULL;
	}
	rb->capacity   = capacity;
	rb->head       = 0;
	rb->tail       = 0;
	rb->count      = 0;
	return rb;
}

void rb_destroy(ringbuf_t *rb)
{
	if (!rb)
		return;
	free(rb->buf);
	free(rb);
}

/*------------------------------------------------------------------*/
/*  capacity / size                                                 */
/*------------------------------------------------------------------*/

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
	rb->head  = 0;
	rb->tail  = 0;
	rb->count = 0;
}

static size_t _free(const ringbuf_t *rb) { return rb->capacity - rb->count; }

/*------------------------------------------------------------------*/
/*  write (append – oldest-first FIFO)                              */
/*------------------------------------------------------------------*/

size_t rb_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
	size_t to_write;
	size_t first;	/* bytes that fit from head to end of ring */
	size_t second;	/* wrapped bytes at the begining               */

	if (!rb || !data || len == 0)
		return 0;

	to_write = len;
	if (to_write > _free(rb))
		to_write = _free(rb);
	if (to_write == 0)
		return 0;

	first   = rb->capacity - rb->head;		/* contiguous tail region */
	if (to_write <= first) {				/* fits without wrap       */
		memcpy(rb->buf + rb->head, data, to_write);
		rb->head += to_write;
		return to_write;
	}

	/* Two-regions write (wrap round): [head..end-1] then [0..remainder]   */
	memcpy(rb->buf + rb->head, data, first);
	second = to_write - first;
	memcpy(rb->buf, data + first, second);
	rb->head  = second;			/* head now sits after the wrapped part */
	return to_write;
}

/*------------------------------------------------------------------*/
/*  read (extract – oldest index first)                             */
/*------------------------------------------------------------------*/

size_t rb_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
	size_t to_read;
	size_t first;
	size_t second;

	if (!rb || !out || len == 0)
		return 0;
	if (rb->count == 0)
		return 0;

	to_read = len;
	if (to_read > rb->count)
		to_read = rb->count;
	if (to_read == 0)
		return 0;

	first   = rb->capacity - rb->tail;	/* contiguous region from tail to end */
	if (to_read <= first) {			/* fits without wrap                   */
		memcpy(out, rb->buf + rb->tail, to_read);
	} else {				/* two-regions: [tail..end-1] then [0..] */
		first = rb->capacity - rb->tail;
		second = to_read - first;
		memcpy(out,        rb->buf + rb->tail, first);
		memcpy(out + first, rb->buf, second);
	}
	rb->tail  += to_read;
	if (rb->tail >= rb->capacity)
		rb->tail -= rb->capacity;
	rb->count -= to_read;
	return to_read;
}

/*------------------------------------------------------------------*/
/*  peek (read without removing)                                    */
/*------------------------------------------------------------------*/

size_t rb_peek(const ringbuf_t *rb, uint8_t *out, size_t len)
{
	size_t to_read;
	size_t first;
	size_t second;

	if (!rb || !out || len == 0)
		return 0;

	to_read = rb->count;
	if (to_read == 0 || len == 0)
		return 0;
	if (len < to_read)
		to_read = len;

	first   = rb->capacity - rb->tail;
	if (to_read <= first) {
		memcpy(out, rb->buf + rb->tail, to_read);
	} else {
		second = to_read - first;
		memcpy(out,        rb->buf + rb->tail, first);
		memcpy(out + first, rb->buf, second);
	}
	return to_read;
}
