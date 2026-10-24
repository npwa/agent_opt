#include "ringbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int passed = 0, failed = 0;

static void check_eq(size_t got, size_t want, const char *desc)
{
    if (got == want) {
        printf("  PASS: %s\n", desc);
        passed++;
    } else {
        printf("  FAIL: %s (got %zu, wanted %zu)\n", desc, got, want);
        failed++;
    }
}

int main(void)
{
    /* 1. Create / capacity / size on new buffer */
    {
        ringbuf_t *rb = rb_create(16);
        check_eq(rb == NULL, 0, "create non-NULL returns rb");
        check_eq(rb_capacity(rb), (size_t)16, "capacity is 16");
        check_eq(rb_size(rb), (size_t)0, "new buffer is empty");
        rb_destroy(rb);
    }

    /* 2. Create with capacity == 0 */
    {
        ringbuf_t *rb = rb_create(0);
        check_eq(rb == NULL, 1, "zero capacity returns NULL");
    }

    /* 3. NULL operations are safe */
    {
        uint8_t buf[16];
        check_eq(rb_write(NULL, buf, 4), (size_t)0, "rb_write(NULL) returns 0");
        check_eq(rb_read(NULL, buf, 4), (size_t)0, "rb_read(NULL) returns 0");
        check_eq(rb_peek(NULL, buf, 4), (size_t)0, "rb_peek(NULL) returns 0");
        check_eq(rb_size(NULL), (size_t)0, "rb_size(NULL) returns 0");
        check_eq(rb_capacity(NULL), (size_t)0, "rb_capacity(NULL) returns 0");
        rb_destroy(NULL); /* shall not crash */
    }

    /* 4. Basic write / read round-trip */
    {
        ringbuf_t *rb = rb_create(16);
        const uint8_t msg[] = {1,2,3,4,5};
        size_t n = rb_write(rb, msg, sizeof(msg));
        check_eq(n, (size_t)5, "writes 5 bytes");
        check_eq(rb_size(rb), (size_t)5, "size is 5 after write");

        uint8_t out[64];
        n = rb_read(rb, out, sizeof(out));
        check_eq(n, (size_t)5, "reads 5 bytes");
        check_eq(memcmp(out, msg, 5), 0, "read data matches written");
        check_eq(rb_size(rb), (size_t)0, "empty after drain");
        rb_destroy(rb);
    }

    /* 5. Write more than capacity — partially fills */
    {
        ringbuf_t *rb = rb_create(16);
        uint8_t big[32];
        memset(big, 0xAA, sizeof(big));
        size_t n = rb_write(rb, big, sizeof(big));
        check_eq(n, (size_t)16, "partial write returns capacity");
        check_eq(rb_size(rb), (size_t)16, "buffer full");

        /* Second write on full buffer */
        uint8_t more[] = {0xFF};
        n = rb_write(rb, more, 1);
        check_eq(n, (size_t)0, "write on full returns 0");
        rb_destroy(rb);
    }

    /* 6. Peek doesn't remove data */
    {
        ringbuf_t *rb = rb_create(16);
        const uint8_t msg[] = {0x10, 0x20, 0x30};
        rb_write(rb, msg, 3);
        check_eq(rb_size(rb), (size_t)3, "size before peek is 3");

        uint8_t peak[64];
        size_t n = rb_peek(rb, peak, 2);
        check_eq(n, (size_t)2, "peek returns count peeked");
        check_eq(peak[0], (uint8_t)0x10, "peek first byte");
        check_eq(peak[1], (uint8_t)0x20, "peek second byte");

        /* peek doesn't change size */
        check_eq(rb_size(rb), (size_t)3, "size unchanged after peek");
        rb_destroy(rb);
    }

    /* 7. Wrap-around correctness */
    {
        ringbuf_t *rb = rb_create(8);
        /* Fill buffer */
        uint8_t first[8] = {0,1,2,3,4,5,6,7};
        rb_write(rb, first, sizeof(first));
        check_eq(rb_size(rb), (size_t)8, "full at 8");

        /* Drain partial so head wraps past end */
        uint8_t tmp[5];
        rb_read(rb, tmp, 5);
        check_eq(rb_size(rb), (size_t)3, "3 bytes remain");

        /* Write again — should wrap correctly */
        uint8_t second[] = {10,11};
        size_t n = rb_write(rb, second, sizeof(second));
        check_eq(n, (size_t)2, "write after partial drain succeeds");
        check_eq(rb_size(rb), (size_t)5, "size is 5 now");

        /* Read everything and verify content in order */
        uint8_t out[64];
        n = rb_read(rb, out, sizeof(out));
        check_eq(n, (size_t)5, "read all 5 remaining");
        check_eq(out[0], (uint8_t)5, "oldest byte is 5");
        check_eq(out[1], (uint8_t)6, "second oldest is 6");
        check_eq(out[2], (uint8_t)7, "third oldest is 7 (wrapped)");
        check_eq(out[3], (uint8_t)10, "fourth is from second write");
        check_eq(out[4], (uint8_t)11, "fifth is from second write");
        rb_destroy(rb);
    }

    /* 8. Clear */
    {
        ringbuf_t *rb = rb_create(64);
        uint8_t data[] = {9,9,9};
        rb_write(rb, data, 3);
        rb_clear(rb);
        check_eq(rb_size(rb), (size_t)0, "clear empties buffer");
        rb_destroy(rb);
    }

    /* 9. Write exactly one byte less than capacity */
    {
        ringbuf_t *rb = rb_create(7);
        uint8_t data[] = {0x10};
        size_t n = rb_write(rb, data, sizeof(data));
        check_eq(n, (size_t)1, "write 1 byte");
        check_eq(rb_size(rb), (size_t)1, "size is 1");
        uint8_t out[1];
        rb_read(rb, out, 1);
        check_eq(out[0], (uint8_t)0x10, "value correct");
        check_eq(rb_size(rb), (size_t)0, "empty after read");
        rb_destroy(rb);
    }

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
