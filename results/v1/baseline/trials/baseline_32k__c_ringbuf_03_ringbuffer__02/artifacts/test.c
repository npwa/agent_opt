#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

int main(void)
{
    /* NULL-safe operations */
    if (rb_size(NULL) != 0) { fprintf(stderr,"FAIL1\n"); return 1; }
    if (rb_capacity(NULL) != 0) { fprintf(stderr,"FAIL2\n"); return 2; }
    rb_destroy(NULL);
    rb_clear(NULL);

    /* Create / capacity */
    ringbuf_t *rb = rb_create(8);
    if (!rb) { fprintf(stderr,"FAIL3\n"); return 3; }
    if (rb_capacity(rb) != 8) { fprintf(stderr,"FAIL4 cap=%zu\n", rb_capacity(rb)); return 4; }
    if (rb_size(rb) != 0) { fprintf(stderr,"FAIL5 size=%zu\n", rb_size(rb)); return 5; }

    /* Partial write - try to write more than capacity */
    unsigned char msg[] = "Hello, ring!";
    size_t w = rb_write(rb, msg, sizeof(msg) - 1);
    fprintf(stderr,"w=%zu rbufsz=%d\n", w, (int)(sizeof(msg)-1));
    if (w != 8) { fprintf(stderr,"FAIL6 w=%zu\n", w); return 6; }
    if (rb_size(rb) != 8) { fprintf(stderr,"FAIL7 size=%zu\n", rb_size(rb)); return 7; }

    /* Drain it */
    unsigned char rbuf[16];
    size_t r = rb_read(rb, rbuf, sizeof(rbuf));
    fprintf(stderr,"r=%zu rbufsz=%d\n", r, 8);
    if (r != 8) { fprintf(stderr,"FAIL8 r=%zu\n", r); return 8; }
    fprintf(stderr,"read bytes:");
    for(int i=0;i<16;i++) fprintf(stderr," %02x",rbuf[i]);
    fprintf(stderr,"\n");
    if (memcmp(rbuf, msg, 8) != 0) { fprintf(stderr,"FAIL9\n"); return 9; }
    if (rb_size(rb) != 0) { fprintf(stderr,"FAIL10 size=%zu\n", rb_size(rb)); return 10; }

    /* Now test wrap-around */
    unsigned char a[] = {1,2,3};
    unsigned char b[] = {4,5,6,7,8,9};
    w = rb_write(rb, a, sizeof(a));
    fprintf(stderr,"w=a=%zu\n", w);
    if (w != 3) { fprintf(stderr,"FAIL11 w=%zu\n", w); return 11; }
    w = rb_write(rb, b, sizeof(b));
    fprintf(stderr,"w=b=%zu size=%zu\n", w, rb_size(rb));
    if (w != 5) { fprintf(stderr,"FAIL12 w=%zu\n", w); return 12; }
    if (rb_size(rb) != 8) { fprintf(stderr,"FAIL13 size=%zu\n", rb_size(rb)); return 13; }

    /* Read back and verify data order */
    unsigned char check[8];
    r = rb_read(rb, check, sizeof(check));
    fprintf(stderr,"check_r=%zu ",r);
    if (r != 8) { fprintf(stderr,"FAIL14\n"); return 14; }
    fprintf(stderr,"check:");
    for(int i=0;i<8;i++) fprintf(stderr," %02x",check[i]);
    fprintf(stderr,"\n");
    for (int i = 0; i < 8; i++) {
        if (check[i] != (uint8_t)(i + 1)) { fprintf(stderr,"FAIL15 i=%d check=%u exp=%u\n", i, check[i], i+1); return 15; }
    }

    /* Full buffer -> write returns 0 */
    w = rb_write(rb, a, sizeof(a));
    fprintf(stderr,"w_full=%zu\n", w);
    if (w != 0) { fprintf(stderr,"FAIL16 w=%zu\n", w); return 16; }

    /* Empty -> read returns 0 */
    r = rb_read(rb, check, 4);
    if (r != 0) { fprintf(stderr,"FAIL17 r=%zu\n", r); return 17; }

    /* Peek test */
    unsigned char peek[] = {99, 100};
    w = rb_write(rb, peek, sizeof(peek));
    unsigned char pk[4];
    r = rb_peek(rb, pk, sizeof(pk));
    if (r != 2) { fprintf(stderr,"FAIL18 r=%zu\n", r); return 18; }
    if (pk[0] != 99 || pk[1] != 100) { fprintf(stderr,"FAIL19\n"); return 19; }
    if (rb_size(rb) != 2) { fprintf(stderr,"FAIL20 size=%zu\n", rb_size(rb)); return 20; }

    /* Clear then verify capacity preserved */
    rb_clear(rb);
    if (rb_size(rb) != 0) { fprintf(stderr,"FAIL21 size=%zu\n", rb_size(rb)); return 21; }
    if (rb_capacity(rb) != 8) { fprintf(stderr,"FAIL22 cap=%zu\n", rb_capacity(rb)); return 22; }

    /* Invalid create */
    ringbuf_t *bad = rb_create(0);
    if (bad) { fprintf(stderr,"FAIL23\n"); return 23; }

    /* NULL write/read/peek */
    if (rb_write(NULL, peek, 2) != 0) { fprintf(stderr,"FAIL24\n"); return 24; }
    if (rb_read(NULL, pk, 4) != 0) { fprintf(stderr,"FAIL25\n"); return 25; }
    if (rb_peek(NULL, pk, 4) != 0) { fprintf(stderr,"FAIL26\n"); return 26; }

    /* Test with capacity 1 */
    ringbuf_t *tiny = rb_create(1);
    if (!tiny) { fprintf(stderr,"FAIL27\n"); return 27; }
    uint8_t v1 = 42, rv;
    w = rb_write(tiny, &v1, 1);
    if (w != 1) { fprintf(stderr,"FAIL28 w=%zu\n", w); return 28; }
    r = rb_read(tiny, &rv, 1);
    if (r != 1 || rv != 42) { fprintf(stderr,"FAIL29 r=%zu rv=%u\n", r, rv); return 29; }
    rb_destroy(tiny);

    /* Partial write */
    ringbuf_t *rb2 = rb_create(3);
    unsigned char big[] = {10,20,30,40,50};
    w = rb_write(rb2, big, sizeof(big));
    if (w != 3) { fprintf(stderr,"FAIL30 w=%zu\n", w); return 30; }
    r = rb_read(rb2, pk, 5);
    if (r != 3 || pk[0] != 10 || pk[1] != 20 || pk[2] != 30) { fprintf(stderr,"FAIL31\n"); return 31; }
    rb_destroy(rb2);

    /* Double destroy */
    ringbuf_t *rb3 = rb_create(4);
    rb_destroy(rb3);
    rb_destroy(NULL);

    rb_destroy(rb);

    printf("All tests passed\n");
    return 0;
}
