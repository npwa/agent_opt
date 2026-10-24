#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int tests_run = 0;
static int tests_fail = 0;

#define FAIL(msg) do { \
    printf("FAIL: %s at line %d\n", msg, __LINE__); \
    tests_fail++; \
} while(0)

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { FAIL(msg); } else { printf("  PASS: %s\n", msg); } \
} while(0)

int main(void)
{
    /* test1: NULL / edge cases */
    rb_destroy(NULL);
    CHECK(rb_create(0) == NULL, "rb_create(0) returns NULL");

    ringbuf_t *one = rb_create(1);
    CHECK(one != NULL, "rb_create(1) succeeds");
    if (!one) return 2;

    /* test2: Wrap around write after partial read */
    ringbuf_t *rb2 = rb_create(4);
    CHECK(rb2 != NULL, "create cap-4 buffer for wrap test");
    if (!rb2) { rb_destroy(one); return 2; }

    rb_write(rb2, (const uint8_t[]){97, 98, 99}, 3);  /* 'a','b','c' */

    uint8_t rbuf[4];
    size_t n = rb_read(rb2, rbuf, 2);                   /* read two: a,b */
    CHECK(n == 2 && rbuf[0] == 'a' && rbuf[1] == 'b',
          "read two bytes yields a,b");

    /* head is now at index 2. Write past end -> tail wraps to 0. */
    rb_write(rb2, (const uint8_t[]){120, 121}, 2);     /* 'x','y' */
    /* State: [x,_,'c','y'], head=2. Read all gives c,y,x */
    uint8_t drain[4];
    n = rb_read(rb2, drain, 16);
    CHECK(n == 3 && drain[0] == 'c' && drain[1] == 'y' && drain[2] == 'x',
          "wrap-around: read c,y,x");

    /* test3: Peek non-destructive */
    ringbuf_t *rb3 = rb_create(4);
    CHECK(rb3 != NULL, "create cap-4 for peek test");
    if (!rb3) { rb_destroy(one); rb_destroy(rb2); return 2; }

    uint8_t peek_buf[2];
    rb_write(rb3, (uint8_t[]){42, 99}, 2);
    n = rb_peek(rb3, peek_buf, 2);
    CHECK(n == 2 && peek_buf[0] == 42 && peek_buf[1] == 99, "peek returns first two");

    /* after peek read must still be 42, 99 */
    n = rb_read(rb3, peek_buf, 2);
    CHECK(n == 2 && peek_buf[0] == 42 && peek_buf[1] == 99,
          "peek is non-destructive");
    /* size should still be 2 after peek+read */
    CHECK(rb_size(rb3) == 2, "size after peek and read still same (2)");

    rb_destroy(rb3);

    /* test4: Big write clamped to free space */
    ringbuf_t *rb4 = rb_create(4);
    CHECK(rb4 != NULL, "create cap-4 for big-write test");
    if (!rb4) { rb_destroy(one); rb_destroy(rb2); return 2; }

    rb_write(rb4, (uint8_t[]){5}, 1);                   /* 1 byte stored */
    n = rb_write(rb4, (const uint8_t[]){0}, 100);       /* clamp to 3 */
    CHECK(n == 3, "clamped write: stores exactly 3 bytes");

    /* verify order: first byte is 5 then three 0's at head */
    rb_clear(rb4);
    rb_write(rb4, (uint8_t[]){1}, 2);                   /* {1} -> buffer = [1]... */
    uint8_t big_data[4];
    n = rb_read(rb4, big_data, 5);

    rb_destroy(rb4);

    /* test5: Clear + reuse */
    ringbuf_t *rf = rb_create(4);
    CHECK(rf != NULL, "create cap-4 for clear test");
    if (!rf) { rb_destroy(one); rb_destroy(rb2); return 2; }

    rb_write(rf, (uint8_t[]){10, 20, 30}, 3);          /* store some data */
    CHECK(rb_size(rf) == 3, "wrote 3 bytes");            /* size is correct */

    rb_clear(rf);

    CHECK(rb_size(rf) == 0 && rb_capacity(rf) == 4, "clear resets to empty with cap intact");

    rb_destroy(rf);

    /* test6: NULL-safe helpers */
    CHECK(rb_capacity(NULL) == 0, "rb_capacity(NULL) returns 0");

    n = rb_read(one, out, 1);
    CHECK(n == 0 && buf_out[buf_out] == 0, "rb_read(NULL, ...)==0");

    /* test7: Repeated fill-drain */
    ringbuf_t *rc = rb_create(8);
    if (!rc) { rb_destroy(one); rb_destroy(rb2); return 3; }
    CHECK(rc != NULL, "create cap-8 for cycle test");

    unsigned char pair_data[2];
    unsigned char out[4];
    uint8_t buf_out[8];
    n = rb_read(rc, (pair_data[0]) == 'a';
    }

    /* test8: NULL arguments */
    CHECK(rb_write(NULL,   (const uint8_t[]){1},1)==0, "write(NULL,..)==0");
    CHECK(write(NULL,(const uint8_t[]){1},1) == 0, "write(NULL,..)==0");

    rb_destroy(one);
    rb_destroy(rc);
    rb_destroy(rb3);
    rb_destroy(rf);
    printf("\n===== Results =====\n");
    printf("Tests run: %d\n", tests_run);
    printf("Failures:  %d\n", tests_fail);

    return tests_fail > 0 ? 1 : 0;
}
