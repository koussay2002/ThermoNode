#include "fault.h"
#include "convert.h"

static uint16_t last_raw;
static bool     have_last;
static uint32_t frozen_count;   /* mesures brutes identiques consécutives (1 = première) */
static uint32_t valid_run;      /* mesures valides consécutives, pour la reprise */

void fault_reset(void)
{
    have_last = false;
    frozen_count = 0u;
    valid_run = 0u;
}

uint32_t fault_frozen_count(void) { return frozen_count; }

bool fault_detect(int32_t centi, uint16_t raw, bool valid)
{
    bool fault;
    if (have_last && (raw == last_raw)) {
        frozen_count++;
    } else {
        frozen_count = 1u;
    }
    last_raw = raw;
    have_last = true;

    fault = ((centi < TN_TEMP_MIN_CENTI) || (centi > TN_TEMP_MAX_CENTI)) || (frozen_count >= FAULT_FROZEN_N);
    if (!valid) {
        fault = true;             /* code ADC impossible */
    }
    if (fault) {
        valid_run = 0u;
    } else {
        valid_run++;
    }
    return fault;
}

bool fault_recover_ready(void)
{
    return valid_run >= FAULT_RECOVER_N;
}
