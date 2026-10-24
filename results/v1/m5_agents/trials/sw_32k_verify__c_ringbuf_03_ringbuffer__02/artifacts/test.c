#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    int passed = 0, failed = 0;

    /* Test 1: create & capacity */
    {
        ringbuf_t *rb = rb_create(8);
        if (!rb) { printf("FAIL: rb_create(8) == NULL\n"); failed++; goto _next; }
        if (rb_capacity(rb) != 8) { printf("FAIL: capacity != 8\n"); failed++; goto _next; }
        if (rb_size(rb) != 0) { printf("FAIL: size != 0 after create\n"); failed++; goto _next; }
        rb_destroy(rb);
        passed++;
    }

    /* Test 2: write then read round-trip */
    {
        uint8_t in[4] = {10, 20, 30, 40};
        uint8_t out[4];
        ringbuf_t *rb = rb_create(4);
        size_t w = rb_write(rb, in, 4);
        if (w != 4) { printf("FAIL: wrote %zu instead of 4\n", w); failed++; goto _next; }
        size_t r = rb_read(rb, out, 4);
        if (r != 4 || memcmp(out, in, 4)) { printf("FAIL: read mismatch round-trip\n"); failed++; goto _next; }
        rb_destroy(rb);
        passed++;
    }

    /* Test 3: wrap-around write */
    {
        /* Write 6 of capacity-8 → head=6 */
        uint8_t in[6]={100,200,55,77,33,99};
        ringbuf_t *rb = rb_create(8);
        (void)rb_write(rb, in, 6); /* head=6, tail=0, size=6 */

        /* Read first 4, freeing positions 0-3 → tail=4, size=2; free={5,6,7,0,1,2}=6 slots... wait
           Actually we still have head=6, so free space is capacity-size = 8-2 = 6.
           But the usable region goes from head(6) wrapping: 6,7,0,1,2,3 — that's positions
           where tail was (4), so we can write up to position 5 but not past it? No...
           
           In my model head is exclusive "next write slot", tail is the oldest data.
           free = capacity - size = 6 writable slots: positions 6,7,0,1,2,3 = 6 positions before reaching tail(4). */
        uint8_t discard[4];
        rb_read(rb, discard, 4); /* tail=4, size=2 still, head=6 ... wait */
        
        /* Actually after rb_read with 4: rd=min(4,6)=4, reads from tail=0 → positions 0,1,2,3.
           tail=(0+4)%8=4, size=6-4=2, head still=6. Correct. */

        /* Now write 6 more — the buffer currently has: buf[5]=99 at pos5, and what? 
           After initial write: buf[0]=100,f[1]=200,f[2]=55,f[3]=77,f[4]=33,f[5]=99
           head=6, tail=0. 
           Read advances tail to 4 removing buf[0..3]. Remaining: buf[4..5] (33,99).
           Free: positions 6,7,0,1,2,3 = 6 slots. */

        uint8_t buf6[6]={1,2,3,4,5,6};
        size_t w = rb_write(rb, buf6, 6);
        if (w != 6) { printf("FAIL: wrap-write returned %zu (expected 6)\n", w); failed++; goto _next; }

        /* head=(6+6)%8=4. tail=4 size=8 (full). */

        /* Read all — should be: original surviving data (33,99) + new data (1..6) */
        uint8_t final[8];
        size_t r = rb_read(rb, final, 8);
        if (r != 8 || final[0]!=33||final[1]!=99||final[2]!=1||final[3]!=2||final[4]!=3||final[5]!=4||final[6]!=5||final[7]!=6) {
            printf("FAIL: wrap-around data mismatch (r=%zu)\n", r); failed++; goto _next;
        }
        rb_destroy(rb);
        passed++;
    }

    /* Test 4: peek */
    {
        uint8_t in[3] = {5,6,7};
        ringbuf_t *rb = rb_create(8);
        rb_write(rb, in, 3);
        uint8_t out[3];
        size_t p = rb_peek(rb, out, 3);
        if (p != 3 || memcmp(out, in, 3)) { printf("FAIL: peek mismatch\n"); failed++; goto _next; }
        /* Size unchanged after peek */
        if (rb_size(rb) != 3) { printf("FAIL: peek should not change size\n"); failed++; goto _next; }
        rb_destroy(rb);
        passed++;
    }

    /* Test 5: NULL-safe helpers */
    {
        if (rb_create(0) != NULL)   { printf("FAIL: rb_create(0) should be NULL\n"); failed++; goto _next; }
        if (rb_size(NULL) != 0)      { printf("FAIL: rb_size(NULL)\n"); failed++; goto _next; }
        if (rb_capacity(NULL) != 0)  { printf("FAIL: rb_capacity(NULL)\n"); failed++; goto _next; }
        rb_destroy(NULL);             /* should not crash */
        rb_clear(NULL);               /* should not crash */
        uint8_t dummy = 0;
        if (rb_write(NULL, &dummy, 1) != 0) { printf("FAIL: write NULL rb\n"); failed++; goto _next; }
        if (rb_read(NULL, &dummy, 1) != 0){ printf("FAIL: read NULL rb\n"); failed++; goto _next; }
        if (rb_peek(NULL, &dummy, 1) != 0){ printf("FAIL: peek NULL rb\n"); failed++; goto _next; }
        passed++;
    }

    /* Test 6: full buffer then partial read then fill again */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t in[4]; for(int i=0;i<4;i++) in[i]=(uint8_t)i;
        /* Write all 4 to make it full */
        if (rb_write(rb, in, 4) != 4) { printf("FAIL: write 4\n"); failed++; goto _next; }
        /* Try writing more when full — should return 0 or partial */
        uint8_t extra = 99;
        size_t w = rb_write(rb, &extra, 1);
        if (w != 0) { printf("FAIL: write to full returned %zu\n", w); failed++; goto _next; }
        /* Read 2 → now 2 free slots */
        uint8_t out[2];
        rb_read(rb, out, 2);
        /* Now we can write exactly 2 more (wrapping: head=2→3,0) */
        uint8_t new_data[2] = {100, 101};
        w = rb_write(rb, new_data, 2);
        if (w != 2){printf("FAIL: partial fill returned %zu\n",w);failed++;goto _next;}
        /* Read all 4 again */
        uint8_t all[4];
        size_t r = rb_read(rb, all, 4);
        /* Expected: [2,3,100,101] — oldest surviving + new data */
        if(r!=4||all[0]!=2||all[1]!=3||all[2]!=100||all[3]!=101){
            printf("FAIL: read after partial refill\n"); failed++;goto _next;}
        rb_destroy(rb);
        passed++;
    }

    /* Test 7: rb_clear */
    {
        ringbuf_t *rb = rb_create(4);
        uint8_t d[4]={1,2,3,4};
        rb_write(rb,d,4);
        rb_read(rb,d,2); /* size=2, tail at 2 */
        rb_clear(rb);
        if (rb_size(rb)!=0) { printf("FAIL: clear not clearing\n"); failed++; goto _next; }
        if (rb_capacity(rb)!=4) { printf("FAIL: capacity changed after clear\n"); failed++; goto _next; }
        rb_destroy(rb);
        passed++;
    }

_next:
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
