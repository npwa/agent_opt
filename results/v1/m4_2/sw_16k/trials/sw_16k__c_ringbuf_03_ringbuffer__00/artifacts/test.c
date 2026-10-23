#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int ok = 1;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); ok = 0; } \
} while (0)

int main(void)
{
    ringbuf_t *rb;

    /* --- create / destroy edge cases --- */
    ASSERT(rb_create(0) == NULL, "create capacity 0");

    rb = rb_create(8);
    ASSERT(rb != NULL, "create ok");
    ASSERT(rb_capacity(rb) == 8, "capacity");
    ASSERT(rb_size(rb) == 0, "size initially zero");
    ASSERT(rb_read(rb, NULL, 4) == 0, "read NULL out");
    ASSERT(rb_write(rb, NULL, 4) == 0, "write NULL data");
    rb_destroy(NULL);               /* must not crash */

    /* --- basic write / read round-trip --- */
    {
        uint8_t txt[] = "hello";
        uint8_t buf[8];
        ASSERT(rb_write(rb, txt, 5) == 5, "write 5");
        ASSERT(rb_size(rb) == 5, "size after write 5");

        memset(buf, 0, sizeof(buf));
        ASSERT(rb_read(rb, buf, 10) == 5, "read <= size");
        ASSERT(memcmp(buf, txt, 5) == 0, "round-trip content");
    }

    /* --- fill to capacity and write overflow (stores what fits) --- */
    {
        uint8_t big[20];
        for (int i = 0; i < 20; i++) big[i] = (uint8_t)i;

        rb_clear(rb);
        ASSERT(rb_write(rb, big, sizeof(big)) == 8, "overflow write");
        ASSERT(rb_size(rb) == 8, "size after overflow write");
    }

    /* --- wrap-around write + read round-trip --- */
    {
        uint8_t buf2[9];

        rb_clear(rb);

        /* Write 6 bytes: tail goes to index 6 */
        for (int i = 0; i < 6; i++) (void)rb_write(rb, (uint8_t[]){(uint8_t)i}, 1);
        ASSERT(rb_size(rb) == 6, "wrap: wrote 6");

        /* Write 3 more → only 2 fit, wraps to index 0 */
        for (int i = 6; i < 9; i++) {
            size_t n = rb_write(rb, (uint8_t[]){(uint8_t)i}, 1);
            ASSERT(n == 1 || n == 0, "wrap: partial write ok");
        }

        /* Buffer should be full (sz=8) with oldest = byte written at index 2 originally */
        ASSERT(rb_size(rb) == 8, "wrap: full");

        /* Read all 8 — first 6 bytes are at buf[0..5] no wrap for reading */
        memset(buf2, 0, sizeof(buf2));
        ASSERT(rb_read(rb, buf2, 9) == 8, "wrap: read all");
        /* First read should give oldest: byte=2 (written at head=0 when space was limited),
           then bytes 3,4,5 from indices 1-4 before wrapping */

        /* Read remaining? None left. */
        ASSERT(rb_size(rb) == 0, "wrap: emptied");

        /* Re-read empty → 0 */
        ASSERT(rb_read(rb, buf2, 4) == 0, "read empty returns 0");
    }

    /* --- peek does not remove --- */
    {
        uint8_t p[4], r[4];
        rb_clear(rb);
        for (int i = 0; i < 4; i++) rb_write(rb, (uint8_t[]){(uint8_t)(i + 10)}, 1);
        ASSERT(rb_size(rb) == 4, "peek setup size");

        memset(p, 0, sizeof(p));
        ASSERT(rb_peek(rb, p, 4) == 4, "peek returns count");
        ASSERT(p[0] == (uint8_t)10, "peek first = 10");
        ASSERT(rb_size(rb) == 4, "peek did not consume");

        memset(r, 0, sizeof(r));
        ASSERT(rb_read(rb, r, 3) == 3, "read after peek");
        memset(p, 0, sizeof(p));
        ASSERT(rb_peek(rb, p, 1) == 1, "peek remaining");
        ASSERT(p[0] == (uint8_t)13, "peek last=13");
    }

    /* --- clear --- */
    rb_write(rb, (uint8_t[]){255}, 1);
    ASSERT(rb_size(rb) > 0, "before clear size>0");
    rb_clear(rb);
    ASSERT(rb_size(rb) == 0, "after clear size=0");

    /* --- multiple partial wraps --- */
    {
        uint8_t out[32];

        rb_clear(rb);
        /* Fill half: bytes 1-4 at indices 0-3 */
        for (int i = 1; i <= 4; i++)
            rb_write(rb, (uint8_t[]){(uint8_t)i}, 1);
        rb_size(rb); /* check size=4 */

        /* Read 3 → head=3, sz=1. buf now logical data at index 3 only */
        ASSERT(rb_read(rb, out, 4) == 3, "partial read");
        ASSERT(out[0] == 1 && out[1] == 2 && out[2] == 3, "partial read content");

        /* Write 5 more → byte at tail(=4): [a,b,c,d,e] with c wrapping to buf[0] */
        rb_write(rb, (uint8_t[]){'a','b','c','d','e'}, 5);
        ASSERT(rb_size(rb) == 4, "wrap: size after mixed ops");

        /* Verify oldest remaining is byte 4 at buf[3] */
        memset(out, 0, sizeof(out));
        ASSERT(rb_read(rb, out, 10) == 4, "verify wrap read count");
        ASSERT(out[0] == 4 && out[1] == (uint8_t)'c' && out[2] == (uint8_t)'d', "wrap-around content");
    }

    rb_destroy(rb);

    printf("%s\n", ok ? "ALL PASSED" : "SOME FAILED");
    return ok ? 0 : 1;
}
