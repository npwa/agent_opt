#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "ringbuf.h"

static unsigned check(const char *label, int success)
{
    static unsigned tests = 0;
    tests++;
    if (success) {
        printf("PASS %s\n", label);
    } else {
        printf("FAIL %s\n", label);
        abort();
    }
    return tests;
}

int main(void)
{
    /* NULL / invalid capacity */
    assert(rb_create(0) == NULL);
    check("create capacity 0 -> NULL", 1);

    rb_destroy(NULL); /* should not crash */
    check("destroy NULL", 1);

    /* basic create/destroy */
    ringbuf_t *rb = rb_create(4);
    assert(rb != NULL);
    check("create capacity 4", 1);
    assert(rb_capacity(rb) == 4);
    check("capacity reports 4", 1);

    /* size of empty buffer */
    assert(rb_size(rb) == 0);
    check("size empty == 0", 1);

    /* clear on empty */
    rb_clear(rb);
    assert(rb_size(rb) == 0);
    check("clear empty still zero", 1);

    /* read / write NULL */
    {
        uint8_t buf[4];
        assert(rb_read(NULL, buf, 4) == 0);
        check("read NULL rb -> 0", 1);
        assert(rb_write(NULL, buf, 4) == 0);
        check("write NULL rb -> 0", 1);
    }

    /* single byte in/out */
    {
        uint8_t data = 42;
        size_t n = rb_write(rb, &data, 1);
        assert(n == 1);
        check("write 1 byte", 1);
        assert(rb_size(rb) == 1);

        uint8_t out;
        n = rb_read(rb, &out, 1);
        assert(n == 1 && out == 42);
        check("read 1 byte matches", 1);
    }

    /* fill to capacity */
    {
        for (int i = 0; i < 4; i++) {
            uint8_t d = (uint8_t)(i + 10);
            assert(rb_write(rb, &d, 1) == 1);
        }
        check("fill 4 bytes", 1);
        assert(rb_size(rb) == 4);

        /* buffer is full */
        uint8_t d = 99;
        assert(rb_write(rb, &d, 1) == 0);
        check("write to full -> 0", 1);

        /* read all, oldest first */
        for (int i = 0; i < 4; i++) {
            uint8_t out;
            size_t n = rb_read(rb, &out, 1);
            assert(n == 1 && out == (uint8_t)(i + 10));
        }
        check("read all in order", 1);
        assert(rb_size(rb) == 0);
    }

    /* wrap-around test: fill partially, write past the end wraps */
    {
        /* fill with 3 bytes to put_pos = 3, free at pos 2..0 (wrapping) */
        uint8_t fill[] = {1, 2, 3};
        size_t n = rb_write(rb, fill, 3);
        assert(n == 3);
        check("write 3 bytes partial", 1);

        /* write more to trigger wrap */
        uint8_t extra[] = {4, 5, 6, 7};
        n = rb_write(rb, extra, sizeof(extra));
        assert(n == 1);   /* only byte '4' fits; capacity is only 4 */
        check("write wraps and stores what fits", 1);

        /* read should get 1..5 */
        for (int i = 0; i < 4; i++) {
            uint8_t out;
            rb_read(rb, &out, 1);
            assert(out == (uint8_t)(i + 1));
        }
        check("wrap reads correct order", 1);
    }

    /* peek doesn't remove */
    {
        for (int i = 0; i < 2; i++) {
            uint8_t d = (uint8_t)(20 + i);
            rb_write(rb, &d, 1);
        }
        uint8_t peeked[2];
        size_t n = rb_peek(rb, peeked, 5);
        assert(n == 2 && peeked[0] == 20 && peeked[1] == 21);
        check("peek reads without removing", 1);
        assert(rb_size(rb) == 2); /* size must be unchanged */

        rb_read(rb, peeked, 2); /* drain */
    }

    /* clear removes everything */
    {
        for (int i = 0; i < 4; i++) {
            uint8_t d = (uint8_t)(30 + i);
            rb_write(rb, &d, 1);
        }
        assert(rb_size(rb) == 4);
        rb_clear(rb);
        assert(rb_size(rb) == 0);
        check("clear empties buffer", 1);
    }

    /* free and re-use same capacity */
    rb_destroy(rb);
    ringbuf_t *rb2 = rb_create(8);
    assert(rb_capacity(rb2) == 8);
    uint8_t w[] = {10, 20};
    assert(rb_write(rb2, w, 2) == 2);
    assert(rb_size(rb2) == 2);
    check("re-created capacity", 1);
    rb_destroy(rb2);

    /* multi-byte burst with wrap */
    ringbuf_t *rb3 = rb_create(4);
    uint8_t buf_write[]  = {100, 101, 102, 103, 104, 105};
    uint8_t buf_read[6];
    assert(rb_write(rb3, buf_write, sizeof(buf_write)) == 4); /* only fits 4 */
    size_t got = rb_read(rb3, buf_read, 4);
    assert(got == 4 && buf_read[0] == 100 && buf_read[3] == 103);
    check("burst writes correct order", 1);
    rb_destroy(rb3);

    printf("\nAll tests passed.\n");
    return 0;
}
