/* hal_double.h — doublures de test du HAL : stub ADC, fake clock, spy radio, injection de coupure */
#ifndef TN_HAL_DOUBLE_H
#define TN_HAL_DOUBLE_H
#include "types.h"

void           hal_double_reset(void);
void           hal_double_push_adc(uint16_t v);          /* séquence rejouée par hal_adc_read */
void           hal_double_advance_ms(uint32_t d);        /* horloge simulée */
void           hal_double_set_connected(bool c);         /* simule la perte de liaison */
int            hal_double_send_calls(void);              /* nombre d'appels réussis à hal_radio_send */
int            hal_double_ack_calls(void);
const uint8_t *hal_double_last_frame(size_t *len);
const uint8_t *hal_double_last_ack(size_t *len);
uint32_t       hal_double_last_send_ms(void);
/* trame n (0 = première) enregistrée, NULL si absente ; au plus 64 trames conservées */
const uint8_t *hal_double_frame(int n, size_t *len);
void           hal_double_queue_command(const char *json); /* commande lue par hal_radio_recv */
int            hal_double_watchdog_kicks(void);

#endif
