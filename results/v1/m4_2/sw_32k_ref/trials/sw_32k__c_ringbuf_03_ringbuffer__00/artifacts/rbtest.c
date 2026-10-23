#include "ringbuf.h"
#include <stdio.h>
#include <string.h>

#define ASSERT(cond) do { if (!(cond)) { printf("FAIL: %s at line %d\n", #cond, __LINE__); return 1; } } while(0)

int main(void) {
    /* rb_create / rb_capacity */
    ringbuf_t *rb = rb_create(8);
    ASSERT(rb != NULL);
    ASSERT(rb_capacity(rb) == 8);

    /* full buffer */
    uint8_t w[8] = {0,1,2,3,4,5,6,7};
    size_t n = rb_write(rb, w, 8);
    ASSERT(n == 8);
    ASSERT(rb_size(rb) == 8);

    /* write to full */
    n = rb_write(rb, w, 1);
    ASSERT(n == 0);

    /* rb_read */
    uint8_t r[4];
    n = rb_read(rb, r, 4);
    ASSERT(n == 4);
    ASSERT(memcmp(r, "\x00\x01\x02\x03", 4) == 0);
    ASSERT(rb_size(rb) == 4);

    /* peek (does not remove data) */
    n = rb_peek(rb, r, 4);
    ASSERT(n == 4);
    ASSERT(memcmp(r, "\x04\x05\x06\x07", 4) == 0);
    ASSERT(rb_size(rb) == 4);

    /* wrap-around write */
    n = rb_write(rb, w, 8);
    ASSERT(n == 4);  /* only 4 bytes fit before capacity reached */

    uint8_t r2[16];
    memset(r2, 0xFF, sizeof(r2));
    n = rb_read(rb, r2, 16);
    ASSERT(n == 8);
    /* oldest 4 are original \x04-\x07; newest 4 are \x00-\x03 */
    for (int i = 0; i < 4; ++i)   ASSERT(r2[i] == (uint8_t)(4 + i));
    for (int i = 4; i < 8; ++i)   ASSERT(r2[i] == (uint8_t)(i - 4));

    /* clear */
    rb_clear(rb);
    ASSERT(rb_size(rb) == 0);

    /* destroy NULL is safe */
    rb_destroy(NULL);
    rb_destroy(rb);

    /* invalid create rejects capacity==0 */
    ringbuf_t *rb2 = rb_create(0);
    ASSERT(rb2 == NULL);

    /* NULL reads/writes return 0 */
    n = rb_read(NULL, r, 1);
    ASSERT(n == 0);
    n = rb_write(NULL, w, 1);
    ASSERT(n == 0);

    printf("All tests PASSED\n");
    return 0;
}
