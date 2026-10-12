/* types.h — types partagés ThermoNode (C99) */
#ifndef TN_TYPES_H
#define TN_TYPES_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TN_ID_LEN        8u
#define TN_FRAME_MAX     128u

typedef enum { STATE_INIT = 0, STATE_NORMAL, STATE_ALERT, STATE_FAULT } node_state_t;

typedef struct {
    char         id[TN_ID_LEN];
    uint32_t     seq;
    uint32_t     t;            /* horodatage (s) */
    int32_t      temp_centi;   /* centi-degrés Celsius */
    node_state_t state;
} frame_t;

#endif
