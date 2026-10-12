#include "node.h"
#include "hal.h"
#include "convert.h"
#include "filter.h"
#include "threshold.h"
#include "fault.h"
#include "ringbuf.h"
#include "frame.h"
#include <string.h>
#include <stdio.h>

static node_state_t state;
static uint32_t     seq;
static uint32_t     data_period_ms = NODE_DATA_PERIOD_MS;
static uint32_t     acq_period_ms  = NODE_PERIOD_MIN_MS;
static uint32_t     last_data_ms;
static int32_t      last_filtered;
static char         node_id[TN_ID_LEN] = "TN01";

void node_set_id(const char *id)
{
    strncpy(node_id, id, TN_ID_LEN - 1u);
    node_id[TN_ID_LEN - 1u] = '\0';
}

void node_set_data_period_ms(uint32_t ms) { data_period_ms = ms; }
uint32_t node_acq_period_ms(void) { return acq_period_ms; }
node_state_t node_state(void) { return state; }
uint32_t node_seq(void) { return seq; }

void node_init(int32_t high_centi, int32_t hyst_centi)
{
    state = STATE_INIT;
    seq = 0u;
    last_data_ms = hal_clock_ms();
    last_filtered = 0;
    acq_period_ms = NODE_PERIOD_MIN_MS;
    filter_reset();
    fault_reset();
    ringbuf_reset();
    threshold_set(high_centi, hyst_centi);
}

/* --- émission avec mémorisation (ENF-05) --- */
static int send_frame_now(const frame_t *f)
{
    uint8_t buf[TN_FRAME_MAX];
    size_t n = frame_build(buf, sizeof buf, f);
    if (n == 0u) {
        return -1;
    }
    return hal_radio_send(buf, n);
}

static void flush_buffer(void)
{
    frame_t f;
    while (ringbuf_peek(&f)) {
        if (send_frame_now(&f) != 0) {
            break;                      /* liaison retombée : on garde l'ordre, on réessaiera */
        }
        (void)ringbuf_pop(&f);
    }
}

static void emit(node_state_t st, int32_t temp_centi)
{
    frame_t f;
    memset(&f, 0, sizeof f);
    memcpy(f.id, node_id, TN_ID_LEN);
    f.seq = seq++;
    f.t = hal_clock_ms() / 1000u;
    f.temp_centi = temp_centi;
    f.state = st;
    if (!hal_radio_connected()) {
        (void)ringbuf_push(&f);
        return;
    }
    if (ringbuf_count() > 0u) {
        flush_buffer();
    }
    if (send_frame_now(&f) != 0) {
        (void)ringbuf_push(&f);
    }
    last_data_ms = hal_clock_ms();
}

/* --- machine à états --- */
void node_step(void)
{
    uint16_t raw = hal_adc_read();
    bool valid;
    int32_t centi = temp_from_adc(raw, &valid);
    bool fault = fault_detect(centi, raw, valid);
    uint32_t now = hal_clock_ms();

    hal_watchdog_kick();

    if (state == STATE_INIT) {
        state = STATE_NORMAL;          /* init_ok : la première mesure est traitée en NORMAL */
    }

    switch (state) {
        case STATE_NORMAL:
            if (fault) {
                state = STATE_FAULT;
                emit(STATE_FAULT, last_filtered);
            } else {
                last_filtered = filter_push(centi, valid);
                if (threshold_step(last_filtered, false) == THR_RAISE) {
                    state = STATE_ALERT;
                    emit(STATE_ALERT, last_filtered);        /* EF-04, ENF-01 : trame immédiate */
                }
            }
            break;
        case STATE_ALERT:
            if (fault) {
                state = STATE_FAULT;
                emit(STATE_FAULT, last_filtered);
            } else {
                last_filtered = filter_push(centi, valid);
                if (threshold_step(last_filtered, true) == THR_CLEAR) {
                    state = STATE_NORMAL;
                    emit(STATE_NORMAL, last_filtered);       /* fin d'alerte */
                }
            }
            break;
        case STATE_FAULT:
            if (false) {   /* TODO reprise */
                state = STATE_NORMAL;
                filter_reset();
                threshold_reset_counter();
                last_filtered = filter_push(centi, valid);
                emit(STATE_NORMAL, last_filtered);           /* reprise (EF-05) */
            }
            break;
        default:
            state = STATE_FAULT;                             /* état inconnu : repli sûr */
            break;
    }

    /* EF-06 : trame de données périodique */
    if ((data_period_ms != 0u) && ((now - last_data_ms) >= data_period_ms)) {
        emit(state, last_filtered);
    }
}

/* --- commandes (EF-07, EF-08, EI-03) --- */
static void send_ack(const char *cmd, const char *result, const char *reason)
{
    uint8_t buf[TN_FRAME_MAX];
    int n = snprintf((char *)buf, sizeof buf, "{\"id\":\"%s\",\"ack\":\"%s\",\"result\":\"%s\",\"reason\":\"%s\"}",
                     node_id, cmd, result, reason);
    if ((n > 0) && ((size_t)n < sizeof buf)) {
        (void)hal_radio_send_ack(buf, (size_t)n);
    }
}

void node_poll_commands(void)
{
    uint8_t buf[TN_FRAME_MAX];
    char cmd[24];
    int32_t value;
    int len = hal_radio_recv(buf, sizeof buf - 1u);
    if (len <= 0) {
        return;
    }
    buf[len] = '\0';
    if (!frame_crc_ok(buf, (size_t)len)) {
        return;                                   /* EF-07 : commande au CRC invalide ignorée */
    }
    if (!json_get_str(buf, (size_t)len, "cmd", cmd, sizeof cmd)) {
        send_ack("?", "nack", "format");
        return;
    }
    if (!json_get_int(buf, (size_t)len, "value", &value)) {
        send_ack(cmd, "nack", "format");
        return;
    }
    if (strcmp(cmd, "set_threshold") == 0) {
        if (threshold_set_checked(value * 100)) {
            send_ack(cmd, "ok", "");
        } else {
            send_ack(cmd, "nack", "range");
        }
    } else if (strcmp(cmd, "set_period") == 0) {
        if ((value >= 1) && (value <= 60)) {
            acq_period_ms = (uint32_t)value * 1000u;
            send_ack(cmd, "ok", "");
        } else {
            send_ack(cmd, "nack", "range");
        }
    } else {
        send_ack(cmd, "nack", "unknown");
    }
}
