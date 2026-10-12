/* fault.h — EF-05 détection de mesure hors plage ou figée, reprise après 5 mesures valides */
#ifndef TN_FAULT_H
#define TN_FAULT_H
#include "types.h"

#define FAULT_FROZEN_N   5u
#define FAULT_RECOVER_N  5u

void fault_reset(void);
/* Met à jour les compteurs et renvoie true si la mesure courante déclenche/maintient un défaut :
 * (centi < -40,00) || (centi > 125,00) || (frozen_count >= 5)  [décision MC/DC du cours] */
bool fault_detect(int32_t centi, uint16_t raw, bool valid);
/* true lorsque FAULT_RECOVER_N mesures valides consécutives (non figées) ont été vues */
bool fault_recover_ready(void);
uint32_t fault_frozen_count(void);

#endif
