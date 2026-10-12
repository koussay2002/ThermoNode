/* threshold.h — EF-04 seuil haut, hystérésis, compteur de mesures consécutives ; EF-07 plage */
#ifndef TN_THRESHOLD_H
#define TN_THRESHOLD_H
#include "types.h"

#define THR_CONSECUTIVE   3u
#define THR_MIN_CENTI     2000     /* 20 °C  (EF-07) */
#define THR_MAX_CENTI     10000    /* 100 °C (EF-07) */
#define THR_DEFAULT_HIGH  6000
#define THR_DEFAULT_HYST  500

typedef enum { THR_NONE = 0, THR_RAISE, THR_CLEAR } thr_event_t;

void        threshold_set(int32_t high_centi, int32_t hyst_centi);
bool        threshold_set_checked(int32_t high_centi);      /* EF-07 : rejette hors [20;100] °C */
int32_t     threshold_high(void);
int32_t     threshold_hyst(void);
void        threshold_reset_counter(void);
/* Évalue la température filtrée. alert_active = état ALERTE courant.
 * RAISE après THR_CONSECUTIVE mesures strictement > seuil (hors alerte) ;
 * CLEAR quand filtrée < seuil - hystérésis (en alerte). */
thr_event_t threshold_step(int32_t filtered_centi, bool alert_active);

#endif
