/* convert.h — EF-02 conversion ADC -> température, EF-05 plage physique */
#ifndef TN_CONVERT_H
#define TN_CONVERT_H
#include "types.h"

#define TN_ADC_MAX        4095u
#define TN_TEMP_MIN_CENTI (-4000)   /* -40,00 °C */
#define TN_TEMP_MAX_CENTI (12500)   /* 125,00 °C */

/* T(centi°C) = (ADC * 3,3 / 4096 - 0,5) * 100 ; *valid = false si code impossible ou hors plage */
int32_t temp_from_adc(uint16_t adc, bool *valid);
/* true si -40,00 <= centi <= 125,00 */
bool    range_check(int32_t centi);

#endif
