/* ENF-05 : tampon circulaire de 60 trames — valeurs limites 0 / 1 / 59 / 60 / 61 */
#include "unity.h"
#include "ringbuf.h"

static frame_t mk(uint32_t seq) { frame_t f = {"TN01", seq, 0u, 2500, STATE_NORMAL}; return f; }

void setUp(void)    { ringbuf_reset(); }
void tearDown(void) {}

void test_ENF05_empty_pop_returns_false(void)
{
    frame_t f;
    TEST_ASSERT_FALSE(ringbuf_pop(&f));
    TEST_ASSERT_EQUAL_size_t(0, ringbuf_count());
}
void test_ENF05_one_push_one_pop(void)
{
    frame_t f = mk(7), g;
    TEST_ASSERT_TRUE(ringbuf_push(&f));
    TEST_ASSERT_EQUAL_size_t(1, ringbuf_count());
    TEST_ASSERT_TRUE(ringbuf_pop(&g));
    TEST_ASSERT_EQUAL_UINT32(7, g.seq);
    TEST_ASSERT_EQUAL_size_t(0, ringbuf_count());
}
void test_ENF05_59_and_60_fit(void)
{
    for (uint32_t i = 0; i < 59; i++) {
        frame_t f = mk(i);
        TEST_ASSERT_TRUE(ringbuf_push(&f));
    }
    TEST_ASSERT_EQUAL_size_t(59, ringbuf_count());
    frame_t f59 = mk(59);
    TEST_ASSERT_TRUE(ringbuf_push(&f59));
    TEST_ASSERT_EQUAL_size_t(60, ringbuf_count());
}
void test_ENF05_push_61_keeps_60_and_drops_oldest(void)
{
    for (uint32_t i = 0; i < 60; i++) {
        frame_t f = mk(i);
        TEST_ASSERT_TRUE(ringbuf_push(&f));
    }
    frame_t f60 = mk(60);
    (void)ringbuf_push(&f60);
    TEST_ASSERT_EQUAL_size_t(60, ringbuf_count());
    frame_t g;
    TEST_ASSERT_TRUE(ringbuf_pop(&g));
    TEST_ASSERT_EQUAL_UINT32(1, g.seq);   /* la plus ancienne (seq 0) a été écrasée */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ENF05_empty_pop_returns_false);
    RUN_TEST(test_ENF05_one_push_one_pop);
    RUN_TEST(test_ENF05_59_and_60_fit);
    RUN_TEST(test_ENF05_push_61_keeps_60_and_drops_oldest);
    return UNITY_END();
}
