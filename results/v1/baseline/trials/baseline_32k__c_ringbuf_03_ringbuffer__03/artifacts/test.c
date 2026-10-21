#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

#define TEST(cond, msg) \
    do { if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); return 1; } } while (0)

int main(void)
{
    ringbuf_t *rb;
    uint8_t buf[32];
    int i;

    /* create */
    TEST(rb_create(0) == NULL, "create with capacity==0 returns NULL");
    rb = rb_create(8);
    TEST(rb != NULL && rb_capacity(rb) == 8, "rb_create(capacity=8)");

    /* size after create */
    TEST(rb_size(rb) == 0, "empty buffer has size==0");

    /* write and clear */
    {   uint8_t w[4] = {1,2,3,4};
        rb_write(rb, w, 4);
        TEST(rb_size(rb) == 4, "write 4 bytes -> size=4");
    }

    rb_clear(rb);
    TEST(rb_size(rb) == 0, "clear resets size to 0");
    TEST(rb_capacity(rb) == 8, "capacity unchanged after clear");

    /* basic write/read full loopback, no wraparound */
    {   uint8_t in[16], out[16];
        rb_clear(rb);
        for (i = 0; i < 16; i++) in[i] = (uint8_t)i;
        TEST(rb_write(rb, in, 16) == 8, "write past capacity only writes what fits");

        rb_clear(rb);
        TEST(rb_write(rb, in, 8) == 8, "write 8 of 8 succeeds");
        TEST(rb_size(rb) == 8,     "size after write == 8");
        TEST(rb_write(rb, in+8, 1) == 0, "write when full returns 0");

        {   size_t n = rb_read(rb, out, 16);
            TEST(n == 8, "read all available bytes");
            TEST(memcmp(in, out, 8) == 0, "loopback data matches");
            TEST(rb_size(rb) == 0,     "buffer drained after read");
        }

        TEST(rb_read(rb, buf, 1) == 0, "read from empty returns 0");
    }

    /* partial write when buffer is full */
    {   rb_clear(rb);
        for (i = 0; i < 6; i++) rb_write(rb, &i, 1);
        TEST(rb_size(rb) == 6, "filled 6 of 8 bytes");

        memset(buf, 0xAA, sizeof(buf));
        TEST(rb_write(rb, buf, 8) == 2, "partial write (8 requested, 2 fit) returns count");
        TEST(rb_size(rb) == 8,     "buffer is full after partial write");

        /* drain and verify partial data */
        memset(buf, 0, sizeof(buf));
        {   size_t n = rb_read(rb, buf, 16);
            TEST(n == 8, "drain all 8 bytes");
            TEST(buf[7] == 170, "partial write data survives at last position");
        }

        rb_clear(rb);
    }

    /* wrap-around read path */
    {   rb_clear(rb);
        for (i = 0; i < 8; i++) rb_write(rb, &i, 1);

        TEST(rb_read(rb, buf, 6) == 6, "read 6 of the way");

        { uint8_t x = 99; rb_write(rb, &x, 1); }

        TEST(rb_read(rb, buf, sizeof(buf)) == 2, "remaining bytes cross boundary on read");
    }

    /* peek does not modify state */
    {   rb_clear(rb);
        for (i = 0; i < 5; i++) rb_write(rb, &i, 1);
        TEST(rb_size(rb) == 5, "pre-peek size is 5");

        { uint8_t p[3];
          size_t n = rb_peek(rb, p, sizeof(p));
          TEST(n == 3, "peek returns 3 bytes");
          TEST(p[0] == 0 && p[1] == 1 && p[2] == 2, "peek values correct");
        }

        TEST(rb_size(rb) == 5, "size unchanged after peek");
    }

    /* NULL safety */
    {
        rb_destroy(NULL);
        TEST(rb_size(NULL)         == 0, "rb_size(NULL) == 0");
        TEST(rb_capacity(NULL)     == 0, "rb_capacity(NULL) == 0");
        TEST(rb_write(NULL, buf, 1)  == 0, "rb_write(NULL) == 0");
        TEST(rb_read(NULL,    buf, 1)  == 0, "rb_read(NULL)   == 0");
    }

    rb_destroy(rb);
    printf("ALL TESTS PASSED\n");
    return 0;
}
