#include "hal.h"
#include "hal_double.h"
#include <string.h>

#define MAX_ADC     512
#define MAX_FRAMES  64

static uint16_t adc_values[MAX_ADC];   /* utilisé une fois hal_adc_read complété */
__attribute__((unused)) static uint16_t *adc_values_ref = adc_values;
static size_t   adc_n, adc_i;
static uint32_t fake_ms;
static bool     connected = true;
static uint8_t  frames[MAX_FRAMES][TN_FRAME_MAX];
static size_t   frame_len[MAX_FRAMES];
static int      send_calls;
static uint32_t last_send_ms;
static uint8_t  last_ack[TN_FRAME_MAX];
static size_t   last_ack_len;
static int      ack_calls;
static char     cmd_queue[TN_FRAME_MAX];
static bool     cmd_pending;
static int      wd_kicks;

void hal_double_reset(void)
{
    adc_n = 0u; adc_i = 0u; fake_ms = 0u; connected = true;
    send_calls = 0; last_send_ms = 0u; ack_calls = 0; last_ack_len = 0u;
    cmd_pending = false; wd_kicks = 0;
    memset(frame_len, 0, sizeof frame_len);
}
void hal_double_push_adc(uint16_t v)   { (void)v; /* TODO G3 : mémoriser la séquence */ }
void hal_double_advance_ms(uint32_t d) { (void)d; /* TODO G3 : horloge simulée */ }
void hal_double_set_connected(bool c)  { connected = c; }
int  hal_double_send_calls(void)       { return send_calls; }
int  hal_double_ack_calls(void)        { return ack_calls; }
uint32_t hal_double_last_send_ms(void) { return last_send_ms; }
int  hal_double_watchdog_kicks(void)   { return wd_kicks; }

const uint8_t *hal_double_frame(int n, size_t *len)
{
    if ((n < 0) || (n >= send_calls) || (n >= MAX_FRAMES)) { *len = 0u; return NULL; }
    *len = frame_len[n];
    return frames[n];
}
const uint8_t *hal_double_last_frame(size_t *len)
{
    if (send_calls == 0) { *len = 0u; return NULL; }
    return hal_double_frame(send_calls - 1, len);
}
const uint8_t *hal_double_last_ack(size_t *len) { *len = last_ack_len; return last_ack; }
void hal_double_queue_command(const char *json)
{
    strncpy(cmd_queue, json, sizeof cmd_queue - 1u); cmd_queue[sizeof cmd_queue - 1u] = '\0';
    cmd_pending = true;
}

/* --- implémentation du HAL --- */
uint16_t hal_adc_read(void)
{
    return 931u; /* TODO G3 : rejouer la séquence poussée par hal_double_push_adc */
}
uint32_t hal_clock_ms(void)      { return fake_ms; }
bool     hal_radio_connected(void) { return connected; }
int hal_radio_send(const uint8_t *buf, size_t len)
{
    if (!connected) { return -1; }
    if ((send_calls < MAX_FRAMES) && (len < TN_FRAME_MAX)) {
        memcpy(frames[send_calls], buf, len); frames[send_calls][len] = 0u; frame_len[send_calls] = len;
    }
    send_calls++; last_send_ms = fake_ms;
    return 0;
}
int hal_radio_send_ack(const uint8_t *buf, size_t len)
{
    if (!connected) { return -1; }
    if (len < TN_FRAME_MAX) { memcpy(last_ack, buf, len); last_ack[len] = 0u; last_ack_len = len; }
    ack_calls++;
    return 0;
}
int hal_radio_recv(uint8_t *buf, size_t cap)
{
    size_t n;
    if (!cmd_pending) { return 0; }
    cmd_pending = false;
    n = strlen(cmd_queue);
    if (n > cap) { n = cap; }
    memcpy(buf, cmd_queue, n);
    return (int)n;
}
void hal_watchdog_kick(void) { wd_kicks++; }
