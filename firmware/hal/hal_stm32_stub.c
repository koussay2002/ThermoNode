/* hal_stm32_stub.c — bouchon minimal du HAL pour mesurer la taille des modules applicatifs en
 * compilation croisée (make cross / size-check) sans la bibliothèque STM32CubeG0. Ne pas flasher. */
#include "hal.h"
#include "node.h"
#include "threshold.h"
static volatile uint32_t ticks;
uint16_t hal_adc_read(void) { return 931u; }
uint32_t hal_clock_ms(void) { return ticks; }
bool     hal_radio_connected(void) { return true; }
int      hal_radio_send(const uint8_t *buf, size_t len) { (void)buf; (void)len; return 0; }
int      hal_radio_send_ack(const uint8_t *buf, size_t len) { (void)buf; (void)len; return 0; }
int      hal_radio_recv(uint8_t *buf, size_t cap) { (void)buf; (void)cap; return 0; }
void     hal_watchdog_kick(void) {}
void _start(void)
{
    node_init(THR_DEFAULT_HIGH, THR_DEFAULT_HYST);
    for (;;) { ticks += node_acq_period_ms(); node_step(); node_poll_commands(); }
}
