#include <stdio.h>
#include "ringbuf.h"

int main() {
    ringbuf_t *rb = rb_create(10);
    uint8_t data[] = {1, 2, 3, 4, 5};
    printf("Written: %zu\n", rb_write(rb, data, 5));
    uint8_t read_data[10];
    printf("Read: %zu\n", rb_read(rb, read_data, 5));
    printf("Peek: %zu\n", rb_peek(rb, read_data, 5));
    rb_destroy(rb);
    return 0;
}