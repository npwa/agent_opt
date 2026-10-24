#include <stdio.h>
#include <assert.h>
#include "ringbuf.h"

static int passed = 0;
static int failed = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %-45s", #name); name(); } while(0)
#define ASSERT(cond) do { \
    if (!(cond)) { printf("FAIL (%s:%d): %s\n", __FILE__, __LINE__, #cond); failed++; return; } \
} while(0)
#define ASSERT_EQ(a,b) ASSERT((a) == (b))

TEST(null_safety)
{
    ASSERT(ringbuf_create(0) == NULL);
    ringbuf_destroy(NULL);
    ASSERT(ringbuf_count(NULL) == 0);
    ASSERT(!ringbuf_is_full(NULL));
    ASSERT(!ringbuf_is_empty(NULL));
    ASSERT(ringbuf_capacity(NULL) == 0);
    uint32_t tmp;
    ASSERT(!ringbuf_peek(NULL, 0, &tmp));
    ASSERT(!ringbuf_pop(NULL, &tmp));
}

TEST(create)
{
    ringbuf_t *rb = ringbuf_create(4);
    ASSERT(rb != NULL);
    ASSERT(ringbuf_count(rb) == 0);
    ASSERT(ringbuf_is_empty(rb));
    ASSERT(!ringbuf_is_full(rb));
    ASSERT(ringbuf_capacity(rb) == 4);
    ringbuf_destroy(rb);
}

TEST(push_pop_roundtrip)
{
    ringbuf_t *rb = ringbuf_create(4);
    for (uint32_t i = 1; i <= 4; i++)
        ASSERT(ringbuf_push(rb, i));
    ASSERT(ringbuf_is_full(rb));
    ASSERT(ringbuf_count(rb) == 4);

    uint32_t v, expected = 1;
    for (int i = 0; i < 4; i++) {
        ASSERT(ringbuf_pop(rb, &v));
        ASSERT_EQ(v, expected++);
    }
    ASSERT(ringbuf_is_empty(rb));
    ringbuf_destroy(rb);
}

TEST(overflow_discard)
{
    ringbuf_t *rb = ringbuf_create(3);
    ringbuf_push(rb, 10);
    ringbuf_push(rb, 20);
    ringbuf_push(rb, 30);
    ASSERT(ringbuf_is_full(rb));

    ringbuf_push(rb, 40);
    ASSERT(!ringbuf_is_empty(rb));
    ASSERT(ringbuf_count(rb) == 3);
    ASSERT(ringbuf_is_full(rb));

    uint32_t v;
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 20);
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 30);
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 40);
    ASSERT(ringbuf_is_empty(rb));
    ringbuf_destroy(rb);
}

TEST(peek)
{
    ringbuf_t *rb = ringbuf_create(5);
    ringbuf_push(rb, 100);
    ringbuf_push(rb, 200);
    uint32_t v;
    ASSERT(ringbuf_peek(rb, 0, &v)); ASSERT_EQ(v, 100);
    ASSERT(ringbuf_peek(rb, 1, &v)); ASSERT_EQ(v, 200);
    ASSERT(!ringbuf_peek(rb, 2, &v));
    ASSERT(ringbuf_peek(rb, -1, &v) == 0);
    ringbuf_destroy(rb);
}

TEST(wrap_interleaved)
{
    ringbuf_t *rb = ringbuf_create(3);
    ringbuf_push(rb, 1);
    ringbuf_push(rb, 2);
    ringbuf_push(rb, 3);
    ASSERT(ringbuf_is_full(rb));

    // overflow: buffer becomes [x,20,30,x40] conceptually [2,3,4]
    ringbuf_push(rb, 4);
    uint32_t v;
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 2);
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 3);
    ASSERT(ringbuf_is_empty(rb));

    // tail wraps back to start. push into slot at original position
    ringbuf_push(rb, 5);
    ASSERT(ringbuf_count(rb) == 1);
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 5);
    ASSERT(ringbuf_is_empty(rb));

    ringbuf_destroy(rb);
}

TEST(push_pop_many)
{
    ringbuf_t *rb = ringbuf_create(2);
    for (uint32_t i = 0; i < 100; i++) {
        ringbuf_push(rb, i + 1);
    }
    // only last 2 remain: 99, 100
    uint32_t v;
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 99u);
    ringbuf_pop(rb, &v); ASSERT_EQ(v, 100u);
    ASSERT(ringbuf_is_empty(rb));
    ringbuf_destroy(rb);
}

int main(void)
{
    printf("ringbuf tests:\n");
    RUN(null_safety);             printf("PASS\n"); passed++;
    RUN(create);                  printf("PASS\n"); passed++;
    RUN(push_pop_roundtrip);      printf("PASS\n"); passed++;
    RUN(overflow_discard);        printf("PASS\n"); passed++;
    RUN(peek);                    printf("PASS\n"); passed++;
    RUN(wrap_interleaved);        printf("PASS\n"); passed++;
    RUN(push_pop_many);           printf("PASS\n"); passed++;

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
