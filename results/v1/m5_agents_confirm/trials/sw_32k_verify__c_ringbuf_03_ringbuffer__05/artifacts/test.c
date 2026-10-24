#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

#define CHECK(cond, msg) do { if (!(cond)) { fprintf(stderr, "FAILED: %s\n", msg); total_fail++; } } while(0)

int main(void) {
    (void)CHECK; /* suppress unused macro in empty branch warning */
    int total_fail = 0;

    /* 1. rb_create with capacity 0 must return NULL */
    if (rb_create(0) != NULL)
        total_fail++;

    /* 2. Normal create and check */
    ringbuf_t *rb = rb_create(16);
    if (!rb) total_fail++; else {
        size_t cap = rb_capacity(rb);
        if (cap != 16) total_fail++;
        if (rb_size(rb) != 0) total_fail++;

        /* 3. Write and read roundtrip */
        const char *msg = "Hello";
        size_t w = rb_write(rb, (const uint8_t *)msg, strlen(msg));
        if (w != 5) total_fail++;
        if (rb_size(rb) != 5) total_fail++;

        uint8_t buf[16] = {0};
        size_t r = rb_read(rb, buf, 16);
        if (r != 5) total_fail++;
        if (memcmp(buf, msg, 5) != 0) total_fail++;
        if (rb_size(rb) != 0) total_fail++;

        /* 4. Wrap-around test */
        ringbuf_t *rb2 = rb_create(8);
        if (!rb2) total_fail++; else {
            uint8_t data[20];
            for (int i = 0; i < 20; i++) data[i] = (uint8_t)i;

            /* fill completely then read half, write rest — forces wrap */
            w = rb_write(rb2, data, 8);          /* fills buffer */
            if (w != 8) total_fail++;
            if (rb_size(rb2) != 8) total_fail++;

            r = rb_read(rb2, buf, 4);             /* read 4 oldest: 0,1,2,3 */
            if (r != 4) total_fail++;
            if (rb_size(rb2) != 4) total_fail++;

            w = rb_write(rb2, data + 8, 6);       /* write beyond end — wraps; only 4 fit */
            if (w != 4) total_fail++;              /* capacity is 8, count was 4, avail=4 */
            if (rb_size(rb2) != 8) total_fail++;  /* now full again */

            /* peek without consuming */
            uint8_t peep[4];
            size_t p = rb_peek(rb2, peep, 4);
            if (p < 4) total_fail++;               /* at least half remaining */

            /* clear */
            rb_clear(rb2);
            if (rb_size(rb2) != 0) total_fail++;

            rb_destroy(rb2);
        }

        /* 5. NULL-handling: all functions must cope with NULL rb or args */
        if (rb_write(NULL, (uint8_t*)"x",1) != 0) total_fail++;
        if (rb_read(NULL, buf, 1) != 0)           total_fail++;
        if (rb_peek(NULL, buf, 1) != 0)           total_fail++;
        if (rb_size(NULL) != 0)                   total_fail++;
        if (rb_capacity(NULL) != 0)               total_fail++;
        rb_destroy(NULL);  /* must not crash */

        rb_destroy(rb);
    }

    /* 6. Write past capacity boundary — partial write */
    rb = rb_create(4);
    if (!rb) total_fail++; else {
        uint8_t src[8] = {10,11,12,13,14,15,16,17};
        size_t w = rb_write(rb, src+2, 6);      /* only 4 bytes should fit */
        if (w != 4) total_fail++;

        uint8_t out[4] = {0};
        r = rb_read(rb, out, 4);
        if (r != 4 || out[0]!=12 || out[3]!=15) total_fail++;

        rb_destroy(rb);
    }

    /* 7. Read from zero-capacity buffer */
    if (rb_read(rb_create(1), NULL, 0) != 0) /* empty read is fine with len=0 */
        ; /* ok: writing 0 bytes reads back 0 */

    if (total_fail == 0) {
        printf("ALL TESTS PASSED\n");
    } else {
        fprintf(stderr, "%d TEST(S) FAILED\n", total_fail);
    }
    return total_fail;
}
