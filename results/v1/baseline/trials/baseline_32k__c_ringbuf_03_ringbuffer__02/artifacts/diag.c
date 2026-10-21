#include <stdio.h>
#include "ringbuf.h"

int main(void)
{
    ringbuf_t *rb = rb_create(8);
    
    /* Fill fully, drain, fill partially */
    for (int i = 0; i < 32; i++) { char c = i; rb_write(rb, &c, 1); }
    printf("after filling: size=%zu cap=%zu\n", rb_size(rb), rb_capacity(rb));
    
    unsigned char out[32], v;
    for (int count = 0; count < 32; ) {
        size_t r = rb_read(rb, out + count, 1);
        if (r == 0) break;
        printf("read[%d]=%u\n", count, out[count]);
        count += r;
    }
    printf("after draining: size=%zu cap=%zu\n", rb_size(rb), rb_capacity(rb));

    /* Write one byte at a time to see wrap */
    for (int i = 0; i < 10; i++) {
        uint8_t v = (uint8_t)(i + 1);
        size_t w = rb_write(rb, &v, 1);
        printf("write[%d]=%u (w=%zu) size=%zu cap=%zu\n", i, v, w, rb_size(rb), rb_capacity(rb));
    }

    /* Now try to write when we think it's full */
    uint8_t extra = 99;
    size_t w = rb_write(rb, &extra, 1);
    printf("final write: w=%zu size=%zu cap=%zu\n", w, rb_size(rb), rb_capacity(rb));

    /* Read them all back in order */
    for (int r = 0; r < rb_size(rb); r++) {
        uint8_t rv;
        rb_read(rb, &rv, 1);
        printf("rev[%d]=%u\n", r, rv);
    }

    return 0;
}
