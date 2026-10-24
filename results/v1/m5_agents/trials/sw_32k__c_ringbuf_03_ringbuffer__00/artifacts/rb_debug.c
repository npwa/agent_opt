#include <stdio.h>
#include "ringbuf.h"

int main(void) {
    ringbuf_t *rb = ringbuf_create(3);
    printf("created, cap=%zu count=%zu\n", ringbuf_capacity(rb), ringbuf_count(rb));

    ringbuf_push(rb, 10);
    printf("push(10): count=%zu full=%d empty=%d\n", ringbuf_count(rb), ringbuf_is_full(rb), ringbuf_is_empty(rb));

    ringbuf_push(rb, 20);
    printf("push(20): count=%zu full=%d empty=%d\n", ringbuf_count(rb), ringbuf_is_full(rb), ringbuf_is_empty(rb));

    ringbuf_push(rb, 30);
    printf("push(30): count=%zu full=%d empty=%d\n", ringbuf_count(rb), ringbuf_is_full(rb), ringbuf_is_empty(rb));

    // Now overflow: head==tail==0, count==3
    ringbuf_push(rb, 40);
    printf("push(40): count=%zu full=%d empty=%d\n", ringbuf_count(rb), ringbuf_is_full(rb), ringbuf_is_empty(rb));

    uint32_t v;
    if (ringbuf_pop(rb, &v))
        printf("pop: %u, count=%zu full=%d\n", v, ringbuf_count(rb), ringbuf_is_full(rb));
    if (ringbuf_pop(rb, &v))
        printf("pop: %u, count=%zu full=%d\n", v, ringbuf_count(rb), ringbuf_is_full(rb));

    // Now try push when count < capacity
    ringbuf_push(rb, 99);
    printf("push(99): count=%zu full=%d\n", ringbuf_count(rb), ringbuf_is_full(rb));
    
    if (ringbuf_pop(rb, &v))
        printf("pop: %u, count=%zu\n", v, ringbuf_count(rb));

    ringbuf_destroy(rb);
    puts("completed successfully");
    return 0;
}
