#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ringbuf.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { if (cond) { tests_passed++; } else { tests_failed++; printf("FAIL: %s\n", msg); } } while(0)

int main(void)
{
    ringbuf_t *rb;
    uint8_t out[16];
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    printf("test 1: NULL/edge\n");
    CHECK(rb_create(0) == NULL, "rb_create(0) returns NULL");
    CHECK(rb_size(NULL) == 0, "rb_size(NULL) returns 0");
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL) returns 0");
    rb_destroy(NULL);

    printf("test 2: create\n");
    rb = rb_create(16);
    CHECK(rb != NULL, "rb_create(16) succeeds");
    CHECK(rb_capacity(rb) == 16, "capacity is 16");
    CHECK(rb_size(rb) == 0, "new buffer is empty");

    printf("test 3: write\n");
    size_t w = rb_write(rb, data, sizeof(data));
    CHECK(w == 10, "wrote 10 bytes");
    CHECK(rb_size(rb) == 10, "size is 10 after write");

    printf("test 4: read\n");
    size_t r = rb_read(rb, out, sizeof(out));
    CHECK(r == 10, "read 10 bytes");
    CHECK(memcmp(out, data, 10) == 0, "data matches");

    printf("test 5: clear\n");
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "size is 0 after clear");

    printf("test 6: full write\n");
    {
        uint8_t big[20];
        memset(big, 0xAA, sizeof(big));
        w = rb_write(rb, big, sizeof(big));
        CHECK(w == 16, "write returns bytes that fit (16)");
        CHECK(rb_size(rb) == 16, "buffer is full after overflow write");

        r = rb_read(rb, out, sizeof(out));
        CHECK(r == 16, "read all 16 bytes");
        for (size_t i = 0; i < 16; i++) {
            CHECK(out[i] == 0xAA, "filled data matches");
        }
    }

    printf("test 7: peek\n");
    {
        uint8_t tmp[] = {0x10, 0x20};
        rb_write(rb, tmp, sizeof(tmp));
        size_t p = rb_peek(rb, out, 4);
        CHECK(p == 2, "peek returns correct count");
        CHECK(out[0] == 0x10 && out[1] == 0x20, "peek data matches");
        CHECK(rb_size(rb) == 2, "size unchanged after peek");
    }

    printf("test 8: wrap\n");
    {
        rb_clear(rb);
        uint8_t fill_data[] = {0xF0};
        w = rb_write(rb, fill_data, sizeof(fill_data));
        CHECK(w == 1, "wrote 1 byte");
        CHECK(rb_size(rb) == 1, "size is 1");

        {
            uint8_t wrap[20];
            memset(wrap, 0xBB, sizeof(wrap));
            w = rb_write(rb, wrap, sizeof(wrap));
            CHECK(w == 15, "wrapped write wrote 15 bytes");
        }

        r = rb_read(rb, out, 16);
        CHECK(r == 16, "read 16 wrapped bytes");
        CHECK(out[0] == 0xF0, "first byte is 0xF0 after wrap");
        for (size_t i = 1; i < 16; i++) {
            CHECK(out[i] == 0xBB, "remaining bytes are 0xBB after wrap");
        }
    }

    printf("test 9: empty read\n");
    r = rb_read(rb, out, sizeof(out));
    CHECK(r == 0, "read from empty returns 0");

    printf("test 10: destroy\n");
    rb_destroy(rb);
    rb_destroy(rb);

    printf("%d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
