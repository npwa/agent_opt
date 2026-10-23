#include "ringbuf.h"
#include <stdio.h>

int main() {
    ringbuf_t *rb = rb_create(10);
    if (!rb) {
        fprintf(stderr, "Failed to create ringbuffer\n");
        return 1;
    }

    uint8_t data[] = {1, 2, 3, 4, 5};
    size_t written = rb_write(rb, data, sizeof(data));
    printf("Wrote %zu bytes\n", written);

    uint8_t read_data[10];
    size_t read = rb_read(rb, read_data, sizeof(read_data));
    printf("Read %zu bytes: ", read);
    for (size_t i = 0; i < read; i++)
        printf("%d ", read_data[i]);
    printf("\n");

    rb_destroy(rb);
    return 0;
}