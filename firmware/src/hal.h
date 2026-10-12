/* hal.h — interface d'abstraction matérielle vue par l'application (EI-01, EI-02)
 * Implémentations : hal/hal_stm32.c (cible), hal/hal_sim.c (hôte, MQTT réel),
 *                   test/hal_double.c (doublures pour les tests unitaires). */
#ifndef TN_HAL_H
#define TN_HAL_H
#include "types.h"

uint16_t hal_adc_read(void);                                   /* code ADC 12 bits (0..4095) */
uint32_t hal_clock_ms(void);                                   /* horloge monotone en ms */
bool     hal_radio_connected(void);                            /* liaison broker disponible ? */
int      hal_radio_send(const uint8_t *buf, size_t len);       /* trame data ; 0 = ok, <0 = échec */
int      hal_radio_send_ack(const uint8_t *buf, size_t len);   /* acquittement de commande */
int      hal_radio_recv(uint8_t *buf, size_t cap);             /* commande reçue ; longueur ou 0 */
void     hal_watchdog_kick(void);

#endif
