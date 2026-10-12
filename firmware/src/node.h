/* node.h — machine à états ThermoNode (EF-01, EF-04, EF-05, EF-06, EF-07, ENF-01, ENF-05, EI-03) */
#ifndef TN_NODE_H
#define TN_NODE_H
#include "types.h"

#define NODE_DATA_PERIOD_MS   10000u     /* EF-06 : trame de données toutes les 10 s */
#define NODE_PERIOD_MIN_MS     1000u     /* EF-08 : période d'acquisition 1..60 s */
#define NODE_PERIOD_MAX_MS    60000u

void         node_init(int32_t high_centi, int32_t hyst_centi);
void         node_set_id(const char *id);
/* 0 = désactive les trames périodiques (utile en test) */
void         node_set_data_period_ms(uint32_t ms);
uint32_t     node_acq_period_ms(void);
/* Une itération : lecture ADC, conversion, détection de défaut, filtre, décision, émission */
void         node_step(void);
/* Traite au plus une commande reçue (EF-07, EF-08, EI-03) */
void         node_poll_commands(void);
node_state_t node_state(void);
uint32_t     node_seq(void);

#endif
