/* Machine à états complète avec doublures : EF-04, EF-05, EF-06, EF-07, EF-08, ENF-01, ENF-05, EI-03 */
#include "unity.h"
#include "node.h"
#include "hal.h"
#include "hal_double.h"
#include "frame.h"
#include <string.h>
#include <stdio.h>

#define ADC_25  931u    /* 25,00 °C */
#define ADC_54  1291u   /* 54,02 °C */
#define ADC_55B 1304u   /* 55,06 °C : au bord de l'hystérésis (>= 55) */
#define ADC_70  1490u   /* 70,04 °C */
#define ADC_130 2234u   /* 129,99 °C : hors plage */

void setUp(void)    { hal_double_reset(); node_init(6000, 500); node_set_data_period_ms(0u); }
void tearDown(void) {}

/* alterne adc / adc+1 (0,08 °C) pour ne pas déclencher la détection de capteur figé (EF-05) */
static void feed(uint16_t adc, int n)
{
    for (int i = 0; i < n; i++) {
        hal_double_push_adc((uint16_t)(adc + (uint16_t)(i & 1)));
        hal_double_advance_ms(1000u);
        node_step();
    }
}
/* même valeur brute répétée : sert aux tests de capteur figé */
static void feed_same(uint16_t adc, int n)
{
    for (int i = 0; i < n; i++) { hal_double_push_adc(adc); hal_double_advance_ms(1000u); node_step(); }
}
static void feed_seq(const uint16_t *v, int n)
{
    for (int i = 0; i < n; i++) { hal_double_push_adc(v[i]); hal_double_advance_ms(1000u); node_step(); }
}
static bool last_frame_has(const char *s)
{
    size_t n; const uint8_t *f = hal_double_last_frame(&n);
    return (f != NULL) && (strstr((const char *)f, s) != NULL);
}
static void queue_signed(const char *payload_without_crc)
{
    char c[128]; size_t n = strlen(payload_without_crc);
    snprintf(c, sizeof c, "%.*s,\"crc\":\"%04X\"}", (int)n - 1, payload_without_crc,
             crc16_ccitt_false((const uint8_t *)payload_without_crc, n));
    hal_double_queue_command(c);
}

void test_INIT_goes_to_NORMAL_on_first_step(void)                 /* INIT -> NORMAL */
{
    TEST_ASSERT_EQUAL(STATE_INIT, node_state());
    feed(ADC_25, 1);
    TEST_ASSERT_EQUAL(STATE_NORMAL, node_state());
}

/* filtre à 5 points : 3 filtrées > 60 °C exigent 6 brutes à 70 °C après 5 à 25 °C (34, 43, 52, 61, 70, 70) */
void test_EF04_no_alert_before_three_filtered_above(void)
{
    feed(ADC_25, 5); feed(ADC_70, 5);
    TEST_ASSERT_EQUAL(STATE_NORMAL, node_state());
    TEST_ASSERT_EQUAL_INT(0, hal_double_send_calls());
}
void test_EF04_alert_after_three_filtered_above_single_frame(void) /* NORMAL -> ALERT [émettre] */
{
    feed(ADC_25, 5); feed(ADC_70, 6);
    TEST_ASSERT_EQUAL(STATE_ALERT, node_state());
    TEST_ASSERT_EQUAL_INT(1, hal_double_send_calls());
    TEST_ASSERT_TRUE(last_frame_has("\"state\":\"ALERT\""));
}
void test_EF04_no_second_alert_frame_while_alert(void)              /* boucle sur ALERT */
{
    feed(ADC_25, 5); feed(ADC_70, 10);
    TEST_ASSERT_EQUAL(STATE_ALERT, node_state());
    TEST_ASSERT_EQUAL_INT(1, hal_double_send_calls());
}
void test_EF04_alert_cleared_below_hysteresis_with_end_frame(void)  /* ALERT -> NORMAL [fin d'alerte] */
{
    feed(ADC_25, 5); feed(ADC_70, 6); feed(ADC_54, 5);
    TEST_ASSERT_EQUAL(STATE_NORMAL, node_state());
    TEST_ASSERT_EQUAL_INT(2, hal_double_send_calls());
    TEST_ASSERT_TRUE(last_frame_has("\"state\":\"NORMAL\""));
}
void test_EF04_alert_kept_at_hysteresis_bound(void)                 /* R5 au niveau système */
{
    feed(ADC_25, 5); feed(ADC_70, 6); feed(ADC_55B, 5);
    TEST_ASSERT_EQUAL(STATE_ALERT, node_state());
}
void test_ENF01_alert_frame_within_2s_of_third_filtered_sample(void) /* contrainte temporelle, horloge simulée */
{
    feed(ADC_25, 5); feed(ADC_70, 5);
    hal_double_push_adc(ADC_70); hal_double_advance_ms(1000u);
    uint32_t t3 = hal_clock_ms(); node_step();
    TEST_ASSERT_EQUAL_INT(1, hal_double_send_calls());
    TEST_ASSERT_LESS_THAN_UINT32(t3 + 2000u, hal_double_last_send_ms() + 1u);
}
void test_EF05_out_of_range_enters_fault_with_frame(void)           /* NORMAL -> DÉFAUT (R1) */
{
    feed(ADC_25, 5); feed(ADC_130, 1);
    TEST_ASSERT_EQUAL(STATE_FAULT, node_state());
    TEST_ASSERT_TRUE(last_frame_has("\"state\":\"FAULT\""));
}
void test_EF05_frozen_sensor_enters_fault(void)                     /* capteur figé : 5 valeurs brutes identiques */
{
    feed_same(ADC_25, 4); TEST_ASSERT_EQUAL(STATE_NORMAL, node_state()); feed_same(ADC_25, 1);
    TEST_ASSERT_EQUAL(STATE_FAULT, node_state());
}
void test_EF05_recovers_to_normal_after_5_valid_samples(void)       /* DÉFAUT -> NORMAL [reprise] */
{
    const uint16_t v[] = {ADC_25, 932u, 933u, 934u, 935u};
    feed_seq(v, 5); feed(ADC_130, 1); TEST_ASSERT_EQUAL(STATE_FAULT, node_state());
    const uint16_t w[] = {931u, 932u, 933u, 934u, 935u};
    feed_seq(w, 4); TEST_ASSERT_EQUAL(STATE_FAULT, node_state());
    feed(935u, 1); TEST_ASSERT_EQUAL(STATE_NORMAL, node_state());
    TEST_ASSERT_EQUAL_INT(2, hal_double_send_calls());              /* trame FAULT + trame de reprise */
}
void test_EF05_spike_isolated_is_fault_not_alert(void)
{
    const uint16_t v[] = {931u, 932u, 933u, ADC_130, 934u}; feed_seq(v, 5);
    TEST_ASSERT_EQUAL(STATE_FAULT, node_state());
}
void test_EF06_periodic_data_frame_every_10s(void)
{
    node_set_data_period_ms(NODE_DATA_PERIOD_MS);
    const uint16_t v[] = {931u, 932u, 933u, 934u, 935u, 931u, 932u, 933u, 934u, 935u, 931u};
    feed_seq(v, 11);
    TEST_ASSERT_EQUAL_INT(1, hal_double_send_calls());
    TEST_ASSERT_TRUE(last_frame_has("\"state\":\"NORMAL\""));
}
void test_ENF05_frames_buffered_and_resent_in_order_after_outage(void) /* perte de liaison simulée */
{
    node_set_data_period_ms(1000u);
    const uint16_t v[] = {931u, 932u, 933u, 934u, 935u, 936u, 937u, 938u, 939u, 940u};
    hal_double_set_connected(false);
    feed_seq(v, 10);
    TEST_ASSERT_EQUAL_INT(0, hal_double_send_calls());
    hal_double_set_connected(true);
    feed(931u, 1);
    TEST_ASSERT_EQUAL_INT(11, hal_double_send_calls());
    for (int i = 0; i < 11; i++) {
        size_t n; const uint8_t *f = hal_double_frame(i, &n); char pat[24];
        snprintf(pat, sizeof pat, "\"seq\":%d,", i);
        TEST_ASSERT_NOT_NULL_MESSAGE(strstr((const char *)f, pat), pat);
    }
}
void test_ENF05_buffer_kept_while_still_disconnected(void)
{
    node_set_data_period_ms(1000u); hal_double_set_connected(false);
    feed(931u, 3); TEST_ASSERT_EQUAL_INT(0, hal_double_send_calls());
    feed(931u, 2); TEST_ASSERT_EQUAL_INT(0, hal_double_send_calls());
}
void test_EF07_command_set_threshold_ok_and_ack(void)               /* cas d'utilisation : scénario principal */
{
    queue_signed("{\"cmd\":\"set_threshold\",\"value\":65}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_EQUAL_INT(1, hal_double_ack_calls());
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"result\":\"ok\""));
}
void test_EF07_command_out_of_range_is_nacked(void)                 /* exception : hors plage */
{
    queue_signed("{\"cmd\":\"set_threshold\",\"value\":101}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"reason\":\"range\""));
}
void test_EF07_command_bad_crc_is_ignored_silently(void)            /* exception : CRC invalide */
{
    hal_double_queue_command("{\"cmd\":\"set_threshold\",\"value\":65,\"crc\":\"0000\"}");
    node_poll_commands();
    TEST_ASSERT_EQUAL_INT(0, hal_double_ack_calls());
}
void test_EF08_set_period_changes_acquisition_period(void)
{
    queue_signed("{\"cmd\":\"set_period\",\"value\":5}");
    node_poll_commands();
    TEST_ASSERT_EQUAL_UINT32(5000, node_acq_period_ms());
}
void test_EF08_set_period_out_of_range_is_nacked(void)
{
    queue_signed("{\"cmd\":\"set_period\",\"value\":61}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"reason\":\"range\""));
    TEST_ASSERT_EQUAL_UINT32(1000, node_acq_period_ms());
}
void test_EI03_unknown_command_is_nacked_unknown(void)
{
    queue_signed("{\"cmd\":\"reboot\",\"value\":1}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"reason\":\"unknown\""));
}
void test_EI03_command_without_cmd_is_nacked_format(void)
{
    queue_signed("{\"value\":1}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"reason\":\"format\""));
}
void test_EI03_command_without_value_is_nacked_format(void)
{
    queue_signed("{\"cmd\":\"set_period\"}");
    node_poll_commands();
    size_t n; const uint8_t *a = hal_double_last_ack(&n);
    TEST_ASSERT_NOT_NULL(strstr((const char *)a, "\"reason\":\"format\""));
}
void test_no_command_pending_does_nothing(void)
{
    node_poll_commands();
    TEST_ASSERT_EQUAL_INT(0, hal_double_ack_calls());
}
void test_EF01_default_acquisition_period_is_1000ms(void)
{
    TEST_ASSERT_EQUAL_UINT32(NODE_PERIOD_MIN_MS, node_acq_period_ms());
    TEST_ASSERT_EQUAL_UINT32(1000, node_acq_period_ms());
}
void test_watchdog_kicked_each_step(void)
{
    feed(ADC_25, 3);
    TEST_ASSERT_EQUAL_INT(3, hal_double_watchdog_kicks());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_INIT_goes_to_NORMAL_on_first_step);
    RUN_TEST(test_EF04_no_alert_before_three_filtered_above);
    RUN_TEST(test_EF04_alert_after_three_filtered_above_single_frame);
    RUN_TEST(test_EF04_no_second_alert_frame_while_alert);
    RUN_TEST(test_EF04_alert_cleared_below_hysteresis_with_end_frame);
    RUN_TEST(test_EF04_alert_kept_at_hysteresis_bound);
    RUN_TEST(test_ENF01_alert_frame_within_2s_of_third_filtered_sample);
    RUN_TEST(test_EF05_out_of_range_enters_fault_with_frame);
    RUN_TEST(test_EF05_frozen_sensor_enters_fault);
    RUN_TEST(test_EF05_recovers_to_normal_after_5_valid_samples);
    RUN_TEST(test_EF05_spike_isolated_is_fault_not_alert);
    RUN_TEST(test_EF06_periodic_data_frame_every_10s);
    RUN_TEST(test_ENF05_frames_buffered_and_resent_in_order_after_outage);
    RUN_TEST(test_ENF05_buffer_kept_while_still_disconnected);
    RUN_TEST(test_EF07_command_set_threshold_ok_and_ack);
    RUN_TEST(test_EF07_command_out_of_range_is_nacked);
    RUN_TEST(test_EF07_command_bad_crc_is_ignored_silently);
    RUN_TEST(test_EF08_set_period_changes_acquisition_period);
    RUN_TEST(test_EF08_set_period_out_of_range_is_nacked);
    RUN_TEST(test_EI03_unknown_command_is_nacked_unknown);
    RUN_TEST(test_EI03_command_without_cmd_is_nacked_format);
    RUN_TEST(test_EI03_command_without_value_is_nacked_format);
    RUN_TEST(test_no_command_pending_does_nothing);
    RUN_TEST(test_EF01_default_acquisition_period_is_1000ms);
    RUN_TEST(test_watchdog_kicked_each_step);
    return UNITY_END();
}
