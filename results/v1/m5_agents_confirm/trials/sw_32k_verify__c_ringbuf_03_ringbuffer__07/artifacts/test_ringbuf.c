#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ringbuf.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; } else { tests_failed++; printf("FAIL: %s\n", msg); } \
} while(0)

int main(void)
{
    /* Test 1: create / destroy / capacity */
    ringbuf_t *rb = rb_create(16);
    CHECK(rb != NULL, "create returns non-NULL");
    CHECK(rb_capacity(rb) == 16, "capacity is 16");

    /* Destroy NULL — must not crash */
    rb_destroy(NULL);
    CHECK(1, "destroy(NULL) no-crash");

    /* Create with capacity=0 → NULL */
    ringbuf_t *rb_fail = rb_create(0);
    CHECK(rb_fail == NULL, "create(0) returns NULL");

    /* ================================================================
     * Test 2: basic write / read round-trip
     * ================================================================
     */
    uint8_t send[] = {10, 20, 30, 40, 50};
    size_t w = rb_write(rb, send, sizeof(send));
    CHECK(w == 5, "write returns 5");
    CHECK(rb_size(rb) == 5, "size is 5 after write");

    uint8_t recv[16] = {0};
    size_t r = rb_read(rb, recv, sizeof(recv));
    CHECK(r == 5, "read returns 5");
    CHECK(memcmp(recv, send, 5) == 0, "read data matches write data");
    CHECK(rb_size(rb) == 0, "size is 0 after reading all");

    /* ================================================================
     * Test 3: NULL / empty / full guards
     * ================================================================
     */
    CHECK(rb_read(rb, recv, sizeof(recv)) == 0, "read on empty returns 0");
    CHECK(rb_write(rb, NULL, 1)       == 0, "NULL data to write → 0");
    CHECK(rb_write(rb, send, 0)       == 0, "len=0 → 0");
    CHECK(rb_read(NULL, recv, 1)      == 0, "rb=NULL → 0 read") ;
    CHECK(rb_size(NULL)              == 0, "size(NULL) == 0");
    CHECK(rb_capacity(NULL)          == 0, "capacity(NULL) == 0");

    /* ================================================================
     * Test 4: wrap-around write + read (the critical path)
     * Strategy: fill buffer so head is near the end, then write more.
     * capacity=16; write 10 bytes → head=10, tail=0, count=10
     * Now we have space from offset 10 to 15 (6 slots) and a gap of
     * 10..10 is free once we consume some.
     * Read 4 bytes → tail=4, head=10; usable at tail-side = 16-4=12 but
     * actual usable capacity (count) = 6. Avail=6.
     * Write 7 bytes: first segment = min(16-10, 7)=6 from offset 10..15,
     *   then wraps: part2=1 from offset 0..0. New head = 1. count=13.
     * Read back everything — should get original 6 bytes followed by new 1 byte.
     * ================================================================
     */
    uint8_t filler[] = {1,2,3,4,5,6,7,8,9,10};
    rb_write(rb, filler, sizeof(filler));   // head=10, tail=0, count=10

    /* Read 4 oldest → they are: 1,2,3,4 */
    uint8_t peek_buf[16];
    size_t r2 = rb_read(rb, peek_buf, 4);
    CHECK(r2 == 4, "read 4 from middle fills correctly");
    CHECK(peek_buf[0]==1 && peek_buf[3]==4, "oldest bytes are intact after partial read");

    /* Now wrap-around write: head=10, count=6, avail=10. Write 7 → should store 7 of them */
    uint8_t wrap_write[] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x99};
    w = rb_write(rb, wrap_write, sizeof(wrap_write));
    CHECK(w == 7, "wrap-around write stores all 7");
    // After this: head=(10+6)%16=0 + 1%16 = 1, actually let me recount.
    // part1 = min(16-10, 7) = min(6,7) = 6 → fills offsets 10..15
    // part2 = 1 → fills offset 0. head becomes (10+7)%16 = 1

    r2 = rb_read(rb, peek_buf, sizeof(peek_buf));
    CHECK(r2 == 13, "read back all content after wrap (6 old + 7 new)");
    // First 6 bytes come from remaining part of filler: {5,6,7,8,9,10}
    CHECK(peek_buf[0]==5 && peek_buf[5]==10, "old data survives wrap-read");
    // Next 7 bytes are what we just wrote
    CHECK(peek_buf[6]==0xAA && peek_buf[12]==0x99, "new data after old in read-back");

    /* ================================================================
     * Test 5: peek does NOT consume data
     * ================================================================
     */
    rb_clear(rb);
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    rb_write(rb, data, sizeof(data));
    uint8_t peeked[16] = {0};
    size_t pr = rb_peek(rb, peeked, 4);
    CHECK(pr == 4, "peek returns 4");
    CHECK(memcmp(peeked, data, 4) == 0, "peek content matches write");
    CHECK(rb_size(rb) == 4, "size unchanged after peek");

    /* ================================================================
     * Test 6: write larger than free space — stores what fits, returns count
     * ================================================================
     */
    rb_clear(rb);                             // empty; count=0, head=0, tail=0
    uint8_t big[32];
    memset(big, 0x55, sizeof(big));
    w = rb_write(rb, big, sizeof(big));       // capacity=16 → can only fit 16
    CHECK(w == 16, "over-sized write stores 16");
    CHECK(rb_size(rb) == 16, "full");

    /* ================================================================
     * Test 7: multiple small writes that force wrap-around each time
     * capacity=16; write 3, read 3, write 3, read 3 … 5 cycles → no-op after empties
     * This tests the normal path but with many head/tail advances.
     * ================================================================
     */
    rb_clear(rb);
    uint8_t val = 0x42;
    for (unsigned cycle = 0; cycle < 5; cycle++) {
        memset(&val, (uint8_t)(cycle * 17 + 1), 1);
        w = rb_write(rb, &val, 3);
        CHECK(w == 3, "write 3");
        uint8_t rbuf[4] = {0};
        r2 = rb_read(rb, rbuf, 3);
        CHECK(r2 == 3, "read 3");
        CHECK(memcmp(&val, rbuf, 1) == 0, "data matches across cycle");
    }

    /* ================================================================
     * Test 8: clear then write; read with len=0
     * ================================================================
     */
    rb_clear(rb);
    CHECK(rb_size(rb) == 0, "clear zeroes size");
    r = rb_read(rb, peek_buf, 0);   // len=0 → 0
    CHECK(r == 0, "read with len=0 returns 0");

    /* ================================================================
     * Cleanup & report
     * ================================================================
     */
    rb_destroy(rb);

    printf("\n=== Results ===\n%d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
