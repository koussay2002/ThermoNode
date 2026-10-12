/* hal_sim.c — HAL de simulation hôte : lit un scénario CSV (température ou code ADC, coupures de liaison,
 * commandes) et publie réellement en MQTT (client minimal MQTT 3.1.1, QoS 0) vers un broker.
 * Variables d'environnement : MQTT_HOST (localhost), MQTT_PORT (1883), NODE_ID (TN01), INPUT (scenario.csv),
 * PERIOD_MS (1000). Utilisé par node-sim dans ci/compose.test.yaml (tests d'intégration).
 * Format du scénario (une ligne par mesure) : "25.3" (°C) | "ADC:1490" | "CONN:0" / "CONN:1" | "CMD:{json}" | "# commentaire".
 * Le fichier peut grandir pendant l'exécution ; sans nouvelle ligne, la dernière valeur est répétée. */
#define _POSIX_C_SOURCE 200809L
#include "hal.h"
#include "node.h"
#include "ringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

static int      sock = -1;
static bool     forced_down;
static FILE    *scenario;
static char     node_id[16] = "TN01";
static char     pending_cmd[256];
static bool     cmd_pending;
static uint16_t last_adc = 931u;
static uint32_t sim_period_ms = 1000u;
static uint16_t jitter;                /* alterne 0/1 pour éviter la détection "capteur figé" */

/* --- MQTT minimal ---------------------------------------------------------------------------- */
static int put_str(uint8_t *p, const char *s) { size_t n = strlen(s); p[0] = (uint8_t)(n >> 8); p[1] = (uint8_t)n; memcpy(p + 2, s, n); return (int)n + 2; }
static bool send_all(const uint8_t *b, size_t n) { while (n > 0) { ssize_t w = send(sock, b, n, MSG_NOSIGNAL); if (w <= 0) { return false; } b += w; n -= (size_t)w; } return true; }

static bool mqtt_connect(void)
{
    const char *host = getenv("MQTT_HOST") ? getenv("MQTT_HOST") : "localhost";
    const char *port = getenv("MQTT_PORT") ? getenv("MQTT_PORT") : "1883";
    struct addrinfo hints, *res, *ai;
    uint8_t pkt[128], ack[4];
    int n;
    struct timeval tv = { 2, 0 };
    memset(&hints, 0, sizeof hints); hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0) { return false; }
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock < 0) { continue; }
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == 0) { break; }
        close(sock); sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) { return false; }
    n = 0;
    pkt[n++] = 0x10; pkt[n++] = 0;
    n += put_str(pkt + n, "MQTT"); pkt[n++] = 4; pkt[n++] = 0x02; pkt[n++] = 0; pkt[n++] = 60;
    { char cid[32]; snprintf(cid, sizeof cid, "sim-%s", node_id); n += put_str(pkt + n, cid); }
    pkt[1] = (uint8_t)(n - 2);
    if (!send_all(pkt, (size_t)n)) { close(sock); sock = -1; return false; }
    if (recv(sock, ack, 4, MSG_WAITALL) != 4 || ack[0] != 0x20 || ack[3] != 0) { close(sock); sock = -1; return false; }
    return true;
}

static int mqtt_publish(const char *topic, const uint8_t *payload, size_t len)
{
    uint8_t hdr[4 + 64];
    int n = 0;
    size_t rem = 2 + strlen(topic) + len;
    if (sock < 0) { return -1; }
    hdr[n++] = 0x30;
    do { uint8_t b = (uint8_t)(rem % 128); rem /= 128; if (rem > 0) { b |= 0x80; } hdr[n++] = b; } while (rem > 0);
    n += put_str(hdr + n, topic);
    if (!send_all(hdr, (size_t)n) || !send_all(payload, len)) { close(sock); sock = -1; return -1; }
    return 0;
}

/* --- scénario -------------------------------------------------------------------------------- */
static uint16_t adc_from_temp(double t) { double v = (t / 100.0 + 0.5) * 4096.0 / 3.3; if (v < 0) v = 0; if (v > 4095) v = 4095; return (uint16_t)(v + 0.5); }

static void scenario_next(void)
{
    char line[256];
    clearerr(scenario);
    if (fgets(line, sizeof line, scenario) == NULL) { return; }
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '#' || line[0] == '\0') { scenario_next(); return; }
    if (strncmp(line, "ADC:", 4) == 0)       { last_adc = (uint16_t)atoi(line + 4); }
    else if (strncmp(line, "CONN:", 5) == 0) { forced_down = (atoi(line + 5) == 0); if (forced_down && sock >= 0) { close(sock); sock = -1; } }
    else if (strncmp(line, "CMD:", 4) == 0)  { snprintf(pending_cmd, sizeof pending_cmd, "%s", line + 4); cmd_pending = true; }
    else                                     { last_adc = adc_from_temp(atof(line)); }
}

/* --- HAL ------------------------------------------------------------------------------------- */
uint16_t hal_adc_read(void) { scenario_next(); jitter ^= 1u; return (uint16_t)(last_adc + jitter); }
uint32_t hal_clock_ms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000); }
bool hal_radio_connected(void)
{
    if (forced_down) { return false; }
    if (sock < 0) { (void)mqtt_connect(); }
    return sock >= 0;
}
int hal_radio_send(const uint8_t *buf, size_t len)
{
    char topic[64]; int r;
    snprintf(topic, sizeof topic, "thermonode/%s/data", node_id);
    r = mqtt_publish(topic, buf, len);
    if (r == 0) { char diag[64]; snprintf(topic, sizeof topic, "thermonode/%s/diag", node_id);
                  int n = snprintf(diag, sizeof diag, "{\"buffered\":%u}", (unsigned)ringbuf_count()); mqtt_publish(topic, (const uint8_t *)diag, (size_t)n); }
    return r;
}
int hal_radio_send_ack(const uint8_t *buf, size_t len)
{ char topic[64]; snprintf(topic, sizeof topic, "thermonode/%s/ack", node_id); return mqtt_publish(topic, buf, len); }
int hal_radio_recv(uint8_t *buf, size_t cap)
{
    size_t n;
    if (!cmd_pending) { return 0; }
    cmd_pending = false; n = strlen(pending_cmd); if (n > cap) { n = cap; }
    memcpy(buf, pending_cmd, n); return (int)n;
}
void hal_watchdog_kick(void) {}

/* --- point d'entrée du simulateur ------------------------------------------------------------- */
void board_init(void)
{
    const char *id = getenv("NODE_ID"), *in = getenv("INPUT"), *p = getenv("PERIOD_MS");
    if (id) { strncpy(node_id, id, sizeof node_id - 1); node_set_id(node_id); }
    if (p)  { sim_period_ms = (uint32_t)atoi(p); }
    scenario = fopen(in ? in : "scenario.csv", "r");
    if (scenario == NULL) { perror("scenario"); exit(1); }
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("node-sim %s : broker %s, scénario %s, période %u ms\n", node_id,
           getenv("MQTT_HOST") ? getenv("MQTT_HOST") : "localhost", in ? in : "scenario.csv", sim_period_ms);
}
uint32_t sim_period(void) { return sim_period_ms; }
