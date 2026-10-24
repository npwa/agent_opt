#include "ringbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST(fn)   do { printf("  %-30s", #fn); fn(); puts("OK"); } while(0)
#define CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return; } } while(0)

static void test_create_destroy(void)
{
    ringbuf_t *rb = rb_create(16);
    CHECK(rb != NULL, "create returned NULL");
    CHECK(rb_capacity(rb) == 16, "capacity wrong");
    CHECK(rb_size(rb)      == 0, "size not zero after create");
    rb_destroy(NULL);           /* must be safe */
    rb_destroy(rb);
}

static void test_write_full_roundtrip(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t in[]   = {1,2,3,4,5,6,7,8};
    uint8_t out[8];
    CHECK(rb_write(rb, in, 8)     == (size_t)8, "write partial");
    CHECK(rb_size(rb)             == (size_t)8, "size wrong after full write");

    CHECK(rb_read(rb, out, 8)     == (size_t)8, "read partial");
    CHECK(memcmp(in, out, 8) == 0, "roundtrip data mismatch");
    CHECK(rb_size(rb)             == (size_t)0, "size not zero after read");
    rb_destroy(rb);
}

static void test_wrap_write(void)
{
    /* capacity=4 */
    ringbuf_t *rb = rb_create(4);
    uint8_t buf[8];

    /* Write 8 bytes into a capacity-4 buffer -> should store all 8, wrapping 4 times:
       After first write(4): storage=[A B C D], head=0, tail=0, size=4
       After read(2): storage=[A B C D], head=0, tail=2, size=2  (old positions conceptually freed)
       After second write(6): stores A B at indices 2,3 then C D E F at indices 0,1,2,3? */

    /* Write full -> [A][B][C][D] head=4%4=0 tail=0 size=4 */
    uint8_t w1[] = {'A','B','C','D'};
    CHECK(rb_write(rb, w1, 4) == (size_t)4, "first write");

    /* Read partial -> consume 'A','B' -> head=0 tail=2 size=2 */
    CHECK(rb_read(rb, buf, 2) == (size_t)2, "first read");
    CHECK(buf[0]=='A' && buf[1]=='B', "read content wrong");

    /* remaining: [C][D] at indices 2,3 head=0 tail=2 size=2 free=2 */

    /* Write 4 more -> stores 2 at {2,3} then wraps to store 2 at {0,1}
       storage[2]=E, storage[3]=F, storage[0]=G, storage[1]=H
       head=(0+4)%4=0 tail=2 size=6??  NO: size was 2+4=6 impossible (capacity=4) */

    /* Actually free = capacity - size = 4-2 = 2. So write(4) returns 2. */
    uint8_t w2[] = {'E','F','G','H'};
    CHECK(rb_write(rb, w2, 4) == (size_t)2, "partial write");

    /* storage: [G][H][C][D], head=(0+2)%4=2 tail=2 size=4 */

    /* Read everything back -> C,D,G,H */
    uint8_t rd[4];
    CHECK(rb_read(rb, rd, 4) == (size_t)4, "read all remaining");
    CHECK(rd[0]=='C' && rd[1]=='D' && rd[2]=='G' && rd[3]=='H',
          "data corrupted after wrap: got %c %c %c %c", rd[0],rd[1],rd[2],rd[3]);

    rb_destroy(rb);
}

static void test_exact_wrap_roundtrip(void)
{
    /* Write 4, fill up. Read 3. Write 2 with wrap. Read everything. */
    ringbuf_t *rb = rb_create(4);
    uint8_t buf[6];

    /* [A][B] at indices 0,1 */
    CHECK(rb_write(rb, (uint8_t[]){'A','B'}, 2) == (size_t)2);
    uint8_t d;
    CHECK(rb_read(rb, &d, 1) == (size_t)1);         /* remove A, tail=1 size=1 head=2 free=3 */

    /* [C][D] at indices 2,3 (wrapping to 0)
       wb_store: n1=4-2=2, cnt=2 -> memcpy(&s[2],{C,D},2), no wrap needed since n1>=cnt
       head=(2+2)%4=0, size=1+2=3  */
    CHECK(rb_write(rb, (uint8_t[]){'C','D'}, 2) == (size_t)2);
    /* storage: [C][?][B][D], tail=1, head=0, size=3 -> logical order: B,D,C (oldest first from tail) */

    uint8_t r[6];
    CHECK(rb_read(rb, r, 3) == (size_t)3);
    /* tail was 1: r[0]=storage[1]=B, r[1]=storage[2]%4=B→wait. Let me think...
       Actually after write: size=3 (1 before + 2 new). But old size was 1 and we wrote 2, so size=3.
       Logical data: oldest at tail=1 = B, then D at storage[(1+1)%4]=storage[2]=B? 
       Wait - after read(1): tail=1, head=2, size=1 -> storage has [A][X][C][D]? No... */

    /* Let me re-derive:
       Start: h=0 t=0 sz=0 cap=4. After write "AB": h=2 t=0 sz=2 s=[A B ? ?]
       After read 1: h=2 t=1 sz=1 s=[A B ? ?]
       Write CD (2 bytes): n1=4-2=2, cnt=2 -> memcpy(&s[2],CD,2), h=(2+2)%4=0 sz=3 s=[A B C D]
       Logical tail=1: data = [B][C][D], read all 3 should give B,C,D */

    uint8_t rd2[3];
    CHECK(rb_read(rb, rd2, 6) == (size_t)3);
    CHECK(rd2[0]=='B' && rd2[1]=='C' && rd2[2]=='D', "exact wrap roundtrip failed");
    rb_destroy(rb);
}

static void test_partial_write(void)
{
    ringbuf_t *rb = rb_create(3);
    uint8_t in[] = {1,2,3,4,5};
    CHECK(rb_size(rb)           == (size_t)0, "");
    CHECK(rb_write(rb, in, 5)   == (size_t)3, "partial write count wrong");
    /* drain */
    uint8_t out[3];
    CHECK(rb_read(rb, out, 3) == (size_t)3);
    CHECK(memcmp(in, out, 3)==0, "data after partial write corrupted");
    rb_destroy(rb);
}

static void test_empty_read(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t buf[4];
    CHECK(rb_read(rb, buf, 4) == (size_t)0, "read from empty should return 0");
    rb_destroy(rb);
}

static void test_peek_no_drain(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t in[]  = {10,20,30,40};
    uint8_t peek[4];
    CHECK(rb_write(rb, in, 4)==(size_t)4, "");
    CHECK(rb_peek(rb, peek, 4)==(size_t)4, "peek return wrong");
    CHECK(peek[0]==10 && peek[3]==40, "peek data wrong");
    CHECK(rb_size(rb) == (size_t)4, "peek modified size");
    rb_destroy(rb);
}

static void test_clear(void)
{
    ringbuf_t *rb = rb_create(4);
    CHECK(rb_write(rb, (uint8_t[]){1,2}, 2)==(size_t)2, "");
    rb_clear(rb);
    CHECK(rb_size(rb)==(size_t)0, "clear size");
    uint8_t expected[] = {0x99, 0xaa};
    uint8_t out[2];
    CHECK(rb_write(rb, expected, 2)==(size_t)2, "");
    rb_read(rb, out, 2);
    CHECK(memcmp(expected, out, 2)==0, "clear then write failed");
    rb_destroy(rb);
}

static void test_null_args(void)
{
    ringbuf_t *rb = rb_create(8);
    uint8_t buf[4];
    CHECK(rb_write(NULL,       buf, 1)==(size_t)0, "write NULL");
    CHECK(rb_read(NULL,        buf, 1)==(size_t)0, "read NULL");
    CHECK(rb_peek(NULL,        buf, 1)==(size_t)0, "peek NULL");
    CHECK(rb_size(NULL),       == (size_t)0, "size NULL failed!");
    CHECK(rb_capacity(NULL),   == (size_t)0, "cap NULL");
    rb_destroy(rb);
}

static void test_peek_after_wrap(void)
{
    ringbuf_t *rb = rb_create(4);
    uint8_t buf[4];

    /* [A][B] at 0,1 */
    CHECK(rb_write(rb, (uint8_t[]){'A','B'},2)==(size_t)2,"");
    /* consume A -> tail=1 size=1 head=2 free=3 */
    CHECK(rb_read(rb, buf,1)==(size_t)1, "");
    /* [B] at 1. Write [C,D] -> n1=4-2=2>=2 OK. storage[2]=C,storage[3]=D. head=0 size=3 */

    CHECK(rb_write(rb, (uint8_t[]){'C','D'},2)==(size_t)2,"");
    /* Now peek the 3 oldest: B,C,D */
    uint8_t pk[3];
    CHECK(rb_peek(rb, pk, 3)==(size_t)3, "peek size wrong after wrap");
    CHECK(pk[0]=='B' && pk[1]=='C' && pk[2]=='D', "peek data wrong after wrap");
    /* peek must not mutate */
    CHECK(rb_size(rb)==(size_t)3, "peek changed size after wrap");

    rb_destroy(rb);
}

int main(void)
{
    puts("Ring buffer tests:");
    TEST(test_create_destroy);
    TEST(test_write_full_roundtrip);
    TEST(test_wrap_write     );
    TEST(test_exact_wrap_rt  );
    TEST(test_partial_write   );
    TEST(test_empty_read      );
    TEST(test_peek_no_drain   );
    TEST(test_clear           );
    TEST(test_null_args       );
    TEST(test_peek_after_wrap );
    puts("\nAll done.");
    return 0;
}
