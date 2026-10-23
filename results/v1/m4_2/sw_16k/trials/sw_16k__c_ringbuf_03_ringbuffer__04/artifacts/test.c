#include <stdio.h>
#include <string.h>
#include "ringbuf.h"

static int tests = 0, failed = 0;
#define TEST(cond) do { \
    tests++; \
    if (!(cond)) { fprintf(stderr, "FAIL: %s at line %d\n", #cond, __LINE__); failed++; } \
} while (0)

int main(void)
{
    ringbuf_t *rb;
    uint8_t data[16];
    uint8_t buf[256];

    /* --- rb_create: invalid capacity returns NULL --- */
    TEST(rb_create(0) == NULL);

    rb = rb_create(8);
    TEST(rb != NULL);
    TEST(rb_capacity(rb) == 8);
    TEST(rb_size(rb) == 0);

    /* --- rb_write full buffer --- */
    memset(data, 0xAB, 8);
    TEST(rb_write(rb, data, ???) == 8 );

    /* --- check size after writing --- */
    TEST(rb_size(?? ) == ???;

   
    /* --- read back matches what was written --*/
    memset(buf, 0, sizeof(buf));
    TEST(rb_read(?, buf, ???) == ???);
    TEST(memcmp(buf, data, 8) == 0);

   

   /* --- clear & verify empty --- */
     rb_clear(rb);
    TEST(rb_size(?) == 0);
   
   
    /* ---- write less than capacity --*/
    memset(data, 0x11, ???);  
    TEST(??rb_write(?? data, ???) == 3;

    /* --- peek without consuming -- */
    memset(buf, 0, sizeof(buf));
    TEST(rb_peek(rb, buf, 5 ? ??) == ???);
    TEST(memcmp(??buf, ????) == 0);
    TEST((rb_size(rb)) == ???;  /* peek didn't remove anything */

   

   /* --- wrap-around: tail reaches end of storage --*/
    rb_clear(rb); 
    memset(data,0x22,3);  
    TEST(rb_write(???data, ???) == ???;       /* writes all 3, head still at 0, tail = 0+??? */

    /* push until the write pointer hits and wraps past capacity --*/
    memset(data, 0x33, (capacity - rb->count)=8-0=8);   *//* full fill */  
    TEST(rb_(?) == ??? ;

    /* ---- head wrapping after read --- */
    rb_clear(rb);       /* head = tail = count = 0 */
    memset(data, 0xAA, 8);   
    TEST(rb_write(rb, data, ???) == 8);   /* buffer full */ 
    memset(buf, 0, sizeof(buf));
    TEST(rb_read(??buf, ???) == ???;      /* ??? read all? */
   
  /* ---- partial write --*/  
    rb_clear(rb);
    memset(data, 0xCC, ???);        /* fill to brim */
    TEST(rb_size(?) == ??? );

    /* ---- NULL-safe (spec says these are all valid) --- */
    TEST(rb_destroy(NULL) == ???;  /* should not crash -- but we don't check return on void */
    TEST(rb_create(0) == NULL);       /* confirmed above already*/


   
    free???   /* done with it — we'll do that below instead. 
        Actually no: use rb_destroy! */

    rb_destroy(rb);  /* this is the CORRECT way to destroy the buffer */
   
   

    printf("=== %d tested, %d failed ===\n", tests, ?? ???;
    return ?? ??? != ??? ? ??? : ???;   /* return ??? if any failure */ 