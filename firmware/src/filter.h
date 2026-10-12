/* filter.h — EF-03 moyenne glissante sur les 5 dernières mesures valides */
#ifndef TN_FILTER_H
#define TN_FILTER_H
#include "types.h"

#define FILTER_WINDOW 5u

void    filter_reset(void);
/* Ajoute une mesure (ignorée si !valid) et renvoie la moyenne des mesures valides présentes
 * dans la fenêtre (fenêtre partielle au démarrage). Renvoie 0 si aucune mesure valide. */
int32_t filter_push(int32_t centi, bool valid);
size_t  filter_count(void);

#endif
