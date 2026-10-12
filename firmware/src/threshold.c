#include "threshold.h"

static int32_t  high_centi = THR_DEFAULT_HIGH;
static int32_t  hyst_centi = THR_DEFAULT_HYST;
static uint32_t above;     /* mesures consécutives au-dessus du seuil */

void threshold_set(int32_t high, int32_t hyst)
{
    high_centi = high;
    hyst_centi = hyst;
    above = 0u;
}

bool threshold_set_checked(int32_t high)
{
    if ((high < THR_MIN_CENTI) || (high > THR_MAX_CENTI)) {
        return false;
    }
    high_centi = high;
    return true;
}

int32_t threshold_high(void) { return high_centi; }
int32_t threshold_hyst(void) { return hyst_centi; }
void    threshold_reset_counter(void) { above = 0u; }

thr_event_t threshold_step(int32_t filtered_centi, bool alert_active)
{
    thr_event_t ev = THR_NONE;
    if (alert_active) {
        if (filtered_centi < (high_centi - hyst_centi)) {
            ev = THR_CLEAR;
            above = 0u;
        }
    } else {
        if (filtered_centi > high_centi) {
            above++;
            if (above >= THR_CONSECUTIVE) {
                ev = THR_RAISE;
            }
        }
    }
    return ev;
}
