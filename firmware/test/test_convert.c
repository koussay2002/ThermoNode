/* EF-02, EF-05 : conversion et plage physique – partitions et valeurs limites */
#include "unity.h"
#include "convert.h"

void setUp(void) {}
void tearDown(void) {}

static int32_t conv(uint16_t adc, bool *v) { *v = true; return temp_from_adc(adc, v); }

void test_EF02_adc_0_gives_minus_50_00(void)          /* TC-02-01 */
{ bool v; TEST_ASSERT_EQUAL_INT32(-5000, conv(0u, &v)); TEST_ASSERT_FALSE(v); }

void test_EF02_adc_2048_gives_115_00(void)             /* TC-02-03 : médiane */
{ bool v; TEST_ASSERT_INT32_WITHIN(1, 11500, conv(2048u, &v)); TEST_ASSERT_TRUE(v); }

void test_EF02_adc_931_gives_25_00(void)
{ bool v; TEST_ASSERT_INT32_WITHIN(3, 2500, conv(931u, &v)); TEST_ASSERT_TRUE(v); }

void test_EF02_adc_4095_is_out_of_physical_range(void) /* TC-02-02 */
{ bool v; TEST_ASSERT_EQUAL_INT32(27991, conv(4095u, &v)); TEST_ASSERT_FALSE(v); }

void test_EF05_adc_2172_is_last_valid_value(void)       /* limite : dernière valeur valide (124,99 °C) */
{ bool v; TEST_ASSERT_EQUAL_INT32(12499, conv(2172u, &v)); TEST_ASSERT_TRUE(v); }

void test_EF05_adc_2173_is_just_above_max(void)         /* limite : juste au-dessus de 125 °C */
{ bool v; TEST_ASSERT_EQUAL_INT32(12507, conv(2173u, &v)); TEST_ASSERT_FALSE(v); }

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EF02_adc_0_gives_minus_50_00);
    RUN_TEST(test_EF02_adc_2048_gives_115_00);
    RUN_TEST(test_EF02_adc_931_gives_25_00);
    RUN_TEST(test_EF02_adc_4095_is_out_of_physical_range);
    RUN_TEST(test_EF05_adc_2172_is_last_valid_value);
    RUN_TEST(test_EF05_adc_2173_is_just_above_max);
    return UNITY_END();
}
