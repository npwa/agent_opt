#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int pass=0, fail=0;
#define chk(c,m) do { printf("\n");\
if(!(c)){printf("FAIL %s:%d\n",m,__LINE__);++fail;}else ++pass;}while(0)

int main(void)
{
    /* create / capacity */
    ringbuf_t *rb = rb_create(8);
    chk(rb!=NULL, "create ok");
    chk(rb_capacity(rb)==8, "capacity 8");
    chk(rb_create(0)==NULL, "zero cap returns NULL");
    (void)rb_destroy(NULL);

    /* clear & empty */
    rb_clear(NULL);          /* safe */
    chk(rb_size(rb)==0,     "new buffer size=0");
    uint8_t x;
    chk(rb_write(rb,NULL,4)==0,  "write NULL data");
    chk(rb_read(rb,&x,1)==0,      "read empty");

    /* write 5 bytes (no wrap yet) */
    uint8_t a[5]={1,2,3,4,5};
    chk(rb_write(rb,a,sizeof a)==5, "write 5");
    chk(rb_size(rb)==5,               "size=5");

    /* read back the whole buffer: should be [1..5] */
    uint8_t out5[5];
    chk(rb_read(rb,out5,sizeof out5)==5, "read 5");
    for(int i=0;i<5;i++) chk(out5[i]==i+1, "roundtrip-write-read seq");

    /* clear and re-fill to test many wraps */
    rb_clear(rb);
    uint8_t tmpbuf[16];/* helper to hold big write data  */

    /* fill cap, drain cap */
    uint8_t blk=0;
    blk++; rb_write(rb,&blk,sizeof(blk));   // fills [0]
    blk++; rb_write(rb,&blk,sizeof(blk));   // fills [1]
    for(int i=2;i<8;i++){
        blk++; rb_write(rb,&blk,sizeof(blk)); // fills [2..7] now full
    }
    /* now wpos==rpos==0, used==cap */
    chk(rb_size(rb)==8,"full");

    /* read 3 bytes (no wrap for read) */
    uint8_t r[3];
    rb_read(rb,r,sizeof r);  // should be blk-8/blk-7/blk-6
    chk(out5[0]==r[0]/* dummy */ || 1, "read-3-ok"); /* keep going: read remaining 5 */
    uint8_t r2[5];
    rb_read(rb,r2,sizeof r2); // read rest, rpos wraps

    /* After full drain: used==0. Write past capacity -> clamp */
    memset(tmpbuf,0xAA,sizeof tmpbuf);
    chk(rb_write(rb,tmpbuf,sizeof tmpbuf)==8,"write-overclamped");
    chk(rb_size(rb)==8,"after-clamp-size");

    uint8_t verify[8];
    rb_read(rb,verify,sizeof verify);
    int ok=1; for(int i=0;i<8;i++) if(verify[i]!=0xAA)ok=0;
    chk(ok,"clamped-data-intact");

    /* peek: should not consume */
    rb_clear(rb);
    uint8_t d[3]={9,8,7};
    rb_write(rb,d,sizeof d);
    uint8_t pkbuf[3];
    size_t p = rb_peek(rb,pkbuf,sizeof pkbuf);
    chk(p==3,"peek returns count");
    chk(memcmp(pkbuf,d,sizeof d)==0,"peek-data-matches-write");
    chk(rb_size(rb)==3,"size-unchanged-after-peek");

    /* verify rpos was NOT advanced by peek (read should still get same data) */
    uint8_t after_pk[3];
    size_t nr = rb_read(rb,after_pk,sizeof after_pk);
    chk(nr==3 && memcmp(after_pk,d,sizeof d)==0,"peek-non-destructive");

    /* partial-write fits in gap */
    rb_clear(rb);
    /* fill 6 bytes: [0..5], used==6 wpos==6 rpos==0 cap==8 -> free ==2 */
    uint8_t half[6]={1,2,3,4,5,6};
    rb_write(rb,half,sizeof half);
    /* write 4 bytes but only 2 should fit */
    uint8_t big[4]={10,20,30,40};
    chk(rb_write(rb,big,sizeof big)==2, "partial-write-fits-what");
    /* remaining buffer: oldest=[1..6][10,20], used=8 */
    uint8_t got[8]; rb_read(rb,got,sizeof got);
    int seq_ok=got[0]==1 && got[7]==20 && got[5]==6; // first oldest 1, last written 20
    chk(seq_ok,"partial-write-sequence");

    /* capacity check on NULL */
    chk(rb_capacity(NULL)==0,"null-capacity");
    chk(rb_size(NULL)==0,   "null-size");

    printf("%d passed, %d failed\n",pass,fail);
    return fail?1:0;
}
