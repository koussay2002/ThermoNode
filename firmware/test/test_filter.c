/* EF-03 : moyenne glissante sur 5 mesures valides */
#include "unity.h"
#include "filter.h"

void setUp(void)    { filter_reset(); }
void tearDown(void) {}

void test_EF03_steady_state_five_identical(void)
{
    int32_t r = 0;
    for (int i = 0; i < 5; i++) { r = filter_push(2500, true); }
    TEST_ASSERT_EQUAL_INT32(2500, r);
    TEST_ASSERT_EQUAL_size_t(5, filter_count());
}

void test_EF03_transient_partial_window(void)
{
    TEST_ASSERT_EQUAL_INT32(100, filter_push(100, true));
    TEST_ASSERT_EQUAL_INT32(150, filter_push(200, true));
    TEST_ASSERT_EQUAL_INT32(200, filter_push(300, true));
    TEST_ASSERT_EQUAL_INT32(250, filter_push(400, true));
    TEST_ASSERT_EQUAL_INT32(300, filter_push(500, true));
}

void test_EF03_sliding_drops_oldest_after_six(void)
{
    for (int i = 1; i <= 5; i++) { filter_push(i * 100, true); }
    TEST_ASSERT_EQUAL_INT32(400, filter_push(600, true)); /* (200+300+400+500+600)/5 */
}

void test_EF03_invalid_sample_is_excluded(void)
{
    filter_push(2500, true);
    filter_push(2500, true);
    TEST_ASSERT_EQUAL_INT32(2500, filter_push(30000, false));
    TEST_ASSERT_EQUAL_size_t(2, filter_count());
}

void test_EF03_isolated_spike_stays_below_threshold(void)
{
    filter_push(2500, true);
    filter_push(2500, true);
    filter_push(9000, true);
    filter_push(2500, true);
    int32_t r = filter_push(2500, true);
    TEST_ASSERT_LESS_THAN_INT32(6000, r); /* (2500*4+9000)/5 = 3800 */
}

void test_EF03_empty_filter_returns_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(0, filter_push(0, false));
    TEST_ASSERT_EQUAL_size_t(0, filter_count());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EF03_steady_state_five_identical);
    RUN_TEST(test_EF03_transient_partial_window);
    RUN_TEST(test_EF03_sliding_drops_oldest_after_six);
    RUN_TEST(test_EF03_invalid_sample_is_excluded);
    RUN_TEST(test_EF03_isolated_spike_stays_below_threshold);
    RUN_TEST(test_EF03_empty_filter_returns_zero);
    return UNITY_END();
}
