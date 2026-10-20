#include "convert.h"

bool range_check(int32_t centi)
{
    return (centi >= TN_TEMP_MIN_CENTI) && (centi <= TN_TEMP_MAX_CENTI);
}

int32_t temp_from_adc(uint16_t adc, bool *valid)
{
    int32_t centi;
    if (adc > TN_ADC_MAX) {
        *valid = false;
        return 0;
    }
    centi = (((int32_t)adc * 33000) / 4096) - 5000;
    *valid = range_check(centi);
    return centi;
}
