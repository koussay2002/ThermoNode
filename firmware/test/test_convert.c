/* EF-02, EF-05 : conversion et plage physique — À COMPLÉTER (partie guidée G1) */
#include "unity.h"
#include "convert.h"

void setUp(void) {}
void tearDown(void) {}

void test_EF02_adc_0_gives_minus_50_00(void)          /* TC-02-01 */
{ bool v; TEST_ASSERT_EQUAL_INT32(-5000, temp_from_adc(0u, &v)); TEST_ASSERT_FALSE(v); }

void test_EF02_adc_2048_gives_115_00(void)             /* TC-02-03 : valeur médiane */
{ bool v; TEST_ASSERT_INT32_WITHIN(1, 11500, temp_from_adc(2048u, &v)); TEST_ASSERT_TRUE(v); }

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EF02_adc_0_gives_minus_50_00);
    RUN_TEST(test_EF02_adc_2048_gives_115_00);
    return UNITY_END();
}
