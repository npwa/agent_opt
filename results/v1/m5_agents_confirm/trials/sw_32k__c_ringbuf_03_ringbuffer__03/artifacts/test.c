#include "ringbuf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    /* --- basic create/destroy capacity/size/clear --- */
    {
        ringbuf_t *rb = rb_create(8);
        assert(rb != NULL);
        assert(rb_capacity(rb) == 8);
        assert(rb_size(rb) == 0);

        rb_destroy(NULL);       /* allowed, no crash                  */
        rb_clear(rb);
        assert(rb_size(rb) == 0);
        rb_destroy(rb);
    }

    /* --- write / read roundtrip --- */
    {
        uint8_t data[] = {1, 2, 3, 4, 5};
        ringbuf_t *rb = rb_create(64);
        assert(rb_write(rb, data, sizeof(data)) == 5);
        assert(rb_size(rb) == 5);

        uint8_t buf[10] = {0};
        assert(rb_read(rb, buf, sizeof(buf)) == 5);
        assert(memcmp(buf, data, 5) == 0);
        assert(rb_size(rb) == 0);
        rb_destroy(rb);
    }

    /* --- NULL handling --- */
    {
        ringbuf_t *rb = rb_create(8);
        assert(rb_read(rb, NULL, 1) == 0);
        assert(rb_write(rb, NULL, 1) == 0);
        assert(rb_capacity(rb) == 8);
        assert(rb_peek(rb, NULL, 1) == 0);
        assert(rb_create(0) == NULL);
        rb_destroy(rb);
    }

    /* --- write that exceeds free space --- */
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t a[] = {1, 2};
        rb_write(rb, a, 2);                   /* filled 2 bytes                 */

        uint8_t tmp[32];
        memset(tmp, 'A', sizeof(tmp));
        assert(rb_write(rb, tmp, 32) == 6);   /* only 6 bytes fit               */
        assert(rb_size(rb) == 8);

        uint8_t out[16] = {0};
        assert(rb_read(rb, out, 16) == 8);
        assert(out[0] == 1 && out[1] == 2);   /* oldest first                   */
        for (int i = 2; i < 8; i++)
            assert(out[i] == 'A');
        rb_destroy(rb);
    }

    /* --- wrap-around of internal storage --- */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t a[] = {10, 20};
        assert(rb_write(rb, a, 2) == 2);

        uint8_t out[4] = {0};
        assert(rb_read(rb, out, 2) == 2);     /* consume everything             */

        uint8_t b[] = {1, 2, 3, 4};
        assert(rb_write(rb, b, 4) == 4);
        assert(rb_size(rb) == 4);

        uint8_t out2[4] = {0};
        assert(rb_read(rb, out2, 4) == 4);
        for (int i = 0; i < 4; i++)
            assert(out2[i] == 1 + i);
        rb_destroy(rb);
    }

    /* --- peek does not consume data --- */
    {
        ringbuf_t *rb = rb_create(8);
        uint8_t a[] = {5, 6, 7};
        rb_write(rb, a, 3);

        uint8_t p[3] = {0};
        assert(rb_peek(rb, p, 3) == 3);
        assert(p[0] == 5 && p[1] == 6 && p[2] == 7);
        assert(rb_size(rb) == 3);     /* peek must not reduce size      */
        rb_destroy(rb);
    }

    printf("All tests passed.\n");
    return 0;
}
