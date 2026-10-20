/* EI-02 : CRC-16/CCITT-FALSE et frame JSON ; EI-03 : aide JSON */
#include "unity.h"
#include "frame.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_EI02_crc_check_value_123456789_is_29B1(void) /* oracle : valeur de contrôle normalisée */
{
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt_false((const uint8_t *)"123456789", 9));
}
void test_EI02_crc_of_empty_is_FFFF(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_ccitt_false((const uint8_t *)"", 0));
}
void test_EI02_crc_changes_when_one_byte_changes(void)
{
    TEST_ASSERT_NOT_EQUAL(crc16_ccitt_false((const uint8_t *)"thermonode", 10),
                          crc16_ccitt_false((const uint8_t *)"thermonodE", 10));
}
void test_EI02_frame_has_all_fields_and_valid_crc(void)
{
    uint8_t buf[TN_FRAME_MAX]; frame_t f = {"TN01", 12u, 1700000000u, 2550, STATE_NORMAL};
    size_t n = frame_build(buf, sizeof buf, &f);
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    TEST_ASSERT_EQUAL_STRING_LEN("{\"id\":\"TN01\",\"seq\":12,\"t\":1700000000,\"temp\":25.50,\"state\":\"NORMAL\",\"crc\":\"",
                                 (const char *)buf, 66);
    TEST_ASSERT_TRUE(frame_crc_ok(buf, n));
}
void test_EI02_negative_temperature_is_formatted(void) /* -3,25 °C */
{
    uint8_t buf[TN_FRAME_MAX]; frame_t f = {"TN01", 1u, 0u, -325, STATE_NORMAL};
    size_t n = frame_build(buf, sizeof buf, &f);
    TEST_ASSERT_NOT_NULL(strstr((const char *)buf, "\"temp\":-3.25,"));
    TEST_ASSERT_TRUE(frame_crc_ok(buf, n));
}
void test_EI02_corrupted_frame_fails_crc(void)
{
    uint8_t buf[TN_FRAME_MAX]; frame_t f = {"TN01", 1u, 0u, 2550, STATE_ALERT};
    size_t n = frame_build(buf, sizeof buf, &f);
    buf[10] ^= 0x01u;
    TEST_ASSERT_FALSE(frame_crc_ok(buf, n));
}
void test_EI02_frame_build_returns_0_on_small_buffer_without_overflow(void) /* BVA tampon + sentinelle */
{
    uint8_t buf[9]; buf[8] = 0xA5u;
    frame_t f = {"TN01", 1u, 1700000000u, 2550, STATE_NORMAL};
    TEST_ASSERT_EQUAL_size_t(0, frame_build(buf, 8, &f));
    TEST_ASSERT_EQUAL_HEX8(0xA5, buf[8]);
}
void test_EI02_crc_ok_rejects_frame_without_crc_member(void)
{
    const char *j = "{\"id\":\"TN01\"}";
    TEST_ASSERT_FALSE(frame_crc_ok((const uint8_t *)j, strlen(j)));
}
void test_EI02_crc_ok_rejects_truncated_crc(void)
{
    const char *j = "{\"id\":\"TN01\",\"crc\":\"12\"}";
    TEST_ASSERT_FALSE(frame_crc_ok((const uint8_t *)j, strlen(j)));
}
void test_EI03_json_get_str_rejects_unterminated_and_too_long(void)
{
    char s[4]; const char *j = "{\"cmd\":\"set_threshold\"}";
    TEST_ASSERT_FALSE(json_get_str((const uint8_t *)j, strlen(j), "cmd", s, sizeof s));
    const char *k = "{\"cmd\":\"abc\"}";
    TEST_ASSERT_FALSE(json_get_str((const uint8_t *)k, strlen(k), "cmd", s, sizeof s));
    const char *m = "{\"value\":x}"; int32_t v;
    TEST_ASSERT_FALSE(json_get_int((const uint8_t *)m, strlen(m), "value", &v));
}
void test_EI03_json_helpers(void)
{
    const char *j = "{\"cmd\":\"set_threshold\",\"value\":65,\"crc\":\"0000\"}";
    char s[16]; int32_t v;
    TEST_ASSERT_TRUE(json_get_str((const uint8_t *)j, strlen(j), "cmd", s, sizeof s));
    TEST_ASSERT_EQUAL_STRING("set_threshold", s);
    TEST_ASSERT_TRUE(json_get_int((const uint8_t *)j, strlen(j), "value", &v));
    TEST_ASSERT_EQUAL_INT32(65, v);
    TEST_ASSERT_FALSE(json_get_int((const uint8_t *)j, strlen(j), "absent", &v));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EI02_crc_check_value_123456789_is_29B1);
    RUN_TEST(test_EI02_crc_of_empty_is_FFFF);
    RUN_TEST(test_EI02_crc_changes_when_one_byte_changes);
    RUN_TEST(test_EI02_frame_has_all_fields_and_valid_crc);
    RUN_TEST(test_EI02_negative_temperature_is_formatted);
    RUN_TEST(test_EI02_corrupted_frame_fails_crc);
    RUN_TEST(test_EI02_frame_build_returns_0_on_small_buffer_without_overflow);
    RUN_TEST(test_EI02_crc_ok_rejects_frame_without_crc_member);
    RUN_TEST(test_EI02_crc_ok_rejects_truncated_crc);
    RUN_TEST(test_EI03_json_get_str_rejects_unterminated_and_too_long);
    RUN_TEST(test_EI03_json_helpers);
    return UNITY_END();
}
