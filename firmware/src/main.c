/* main.c — boucle principale (cible : hal_stm32.c ; simulateur hôte : hal_sim.c avec -DTN_SIM) */
#ifdef TN_SIM
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#endif
#include "node.h"
#include "hal.h"
#include "threshold.h"

extern void board_init(void);   /* fournie par hal_stm32.c ou hal_sim.c */
#ifdef TN_SIM
extern uint32_t sim_period(void);
#define ACQ_PERIOD() sim_period()
#else
#define ACQ_PERIOD() node_acq_period_ms()
#endif

int main(void)
{
    uint32_t next;
    board_init();
    node_set_id("TN01");
    node_init(THR_DEFAULT_HIGH, THR_DEFAULT_HYST);
    next = hal_clock_ms();
    for (;;) {
        uint32_t now = hal_clock_ms();
        if ((int32_t)(now - next) >= 0) {
            next += ACQ_PERIOD();                  /* EF-01 : période sans dérive cumulée */
            node_step();
        }
        node_poll_commands();
#ifdef TN_SIM
        { struct timespec ts = {0, 20000000L}; nanosleep(&ts, NULL); }
#endif
    }
}
