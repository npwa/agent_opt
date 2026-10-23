#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuf.h"

int main(void)
{
    ringbuf_t *rb;
    uint8_t data[16];
    size_t r;

    assert(rb_create(0) == NULL);

    rb = rb_create(8);
    assert(rb != NULL);
    assert(rb_capacity(rb) == 8);
    assert(rb_size(rb) == 0);

    /* write 5 bytes */
    memset(data, 0xAA, 5);
    r = rb_write(rb, data, 5);
    assert(r == 5);
    assert(rb_size(rb) == 5);

    /* peek shows same data without removing */
    memset(data, 0, sizeof(data));
    r = rb_peek(rb, data, 3);
    assert(r == 3);
    assert(data[0] == 0xAA && data[1] == 0xAA && data[2] == 0xAA);
    assert(rb_size(rb) == 5);

    /* read 3 bytes back */
    memset(data, 0, sizeof(data));
    r = rb_read(rb, data, 3);
    assert(r == 3);
    assert(data[0] == 0xAA && data[1] == 0xAA && data[2] == 0xAA);
    assert(rb_size(rb) == 2);

    /* write 6 more (wrapping around) */
    memset(data, 0xBB, 6);
    r = rb_write(rb, data, 6);
    assert(r == 6);  /* capacity remaining is 6 */
    assert(rb_size(rb) == 8);

    /* read everything back - should get the 2 remaining AA + all 6 BB */
    memset(data, 0, sizeof(data));
    r = rb_read(rb, data, 8);
    assert(r == 8);
    assert(data[0] == 0xAA && data[1] == 0xAA);  /* 2 remaining original bytes */
    assert(data[2] == 0xBB && data[7] == 0xBB);  /* all 6 new bytes */

    /* clear and verify */
    rb_clear(rb);
    assert(rb_size(rb) == 0);

    memset(data, 0xCC, 4);
    r = rb_write(rb, data, 4);
    assert(r == 4);

    memset(data, 0, sizeof(data));
    r = rb_read(rb, data, 4);
    assert(r == 4);
    assert(data[0] == 0xCC && data[3] == 0xCC);

    /* destroy NULL is OK */
    rb_destroy(NULL);
    rb_destroy(rb);

    /* test write/peek on NULL rb */
    assert(rb_write(NULL, data, 4) == 0);
    assert(rb_read(NULL, data, 4) == 0);
    assert(rb_peek(NULL, data, 4) == 0);
    assert(rb_size(NULL) == 0);
    assert(rb_capacity(NULL) == 0);

    printf("All tests passed.\n");
    return 0;
}
