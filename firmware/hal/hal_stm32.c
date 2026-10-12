/* hal_stm32.c — implémentation du HAL pour la cible NUCLEO-G031K8 (STM32G031K8, Cortex-M0+, 64 Ko / 8 Ko)
 * avec un capteur TMP36 sur PA0 (ADC_IN0) et un module Wi-Fi ESP-01S (firmware ESP-AT >= 2.2, commandes
 * AT+MQTT*) sur USART1 (PA9 = TX, PA10 = RX). Ce fichier s'appuie sur la bibliothèque STM32CubeG0 (HAL) :
 * les handles hadc1, huart1, hiwdg et les fonctions MX_* / SystemClock_Config sont générés par STM32CubeMX
 * (voir docs/guide-construction.md). Compilé UNIQUEMENT dans le projet STM32CubeIDE. */
#include "hal.h"
#include "stm32g0xx_hal.h"
#include <string.h>
#include <stdio.h>

extern ADC_HandleTypeDef  hadc1;
extern UART_HandleTypeDef huart1;
extern IWDG_HandleTypeDef hiwdg;
extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_ADC1_Init(void);
extern void MX_USART1_UART_Init(void);
extern void MX_IWDG_Init(void);

#define WIFI_SSID   "THERMONODE_LAB"
#define WIFI_PASS   "changez-moi"
#define MQTT_HOST   "192.168.1.10"
#define MQTT_PORT   "1883"
#define NODE_ID     "TN01"

static bool    link_up;
static uint8_t rx_buf[256];
static size_t  rx_len;

/* --- utilitaires AT ------------------------------------------------------------------------- */
static bool at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    uint8_t c;
    uint32_t t0 = HAL_GetTick();
    char line[256];
    size_t n = 0u;
    HAL_UART_Transmit(&huart1, (const uint8_t *)cmd, (uint16_t)strlen(cmd), 100);
    HAL_UART_Transmit(&huart1, (const uint8_t *)"\r\n", 2, 100);
    while ((HAL_GetTick() - t0) < timeout_ms) {
        if (HAL_UART_Receive(&huart1, &c, 1, 10) == HAL_OK) {
            if (n < sizeof line - 1u) { line[n++] = (char)c; line[n] = '\0'; }
            if (strstr(line, expect) != NULL) { return true; }
            if (strstr(line, "ERROR") != NULL) { return false; }
        }
    }
    return false;
}

static bool wifi_mqtt_connect(void)
{
    char buf[160];
    if (!at_cmd("AT", "OK", 1000)) { return false; }
    (void)at_cmd("AT+CWMODE=1", "OK", 1000);
    snprintf(buf, sizeof buf, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    if (!at_cmd(buf, "OK", 15000)) { return false; }
    snprintf(buf, sizeof buf, "AT+MQTTUSERCFG=0,1,\"%s\",\"\",\"\",0,0,\"\"", NODE_ID);
    if (!at_cmd(buf, "OK", 2000)) { return false; }
    snprintf(buf, sizeof buf, "AT+MQTTCONN=0,\"%s\",%s,1", MQTT_HOST, MQTT_PORT);
    if (!at_cmd(buf, "OK", 10000)) { return false; }
    snprintf(buf, sizeof buf, "AT+MQTTSUB=0,\"thermonode/%s/cmd\",0", NODE_ID);
    return at_cmd(buf, "OK", 2000);
}

/* --- initialisation carte ------------------------------------------------------------------- */
void board_init(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USART1_UART_Init();
    MX_IWDG_Init();                 /* chien de garde ~2 s, rafraîchi par hal_watchdog_kick() */
    HAL_ADCEx_Calibration_Start(&hadc1);
    link_up = wifi_mqtt_connect();
}

/* --- HAL ------------------------------------------------------------------------------------ */
uint16_t hal_adc_read(void)
{
    uint16_t v = 0xFFFFu;                       /* code impossible si échec : traité comme invalide (EF-05) */
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            v = (uint16_t)HAL_ADC_GetValue(&hadc1);     /* 12 bits, 0..4095, Vref = 3,3 V */
        }
        (void)HAL_ADC_Stop(&hadc1);
    }
    return v;
}

uint32_t hal_clock_ms(void) { return HAL_GetTick(); }

bool hal_radio_connected(void)
{
    if (!link_up) {
        link_up = wifi_mqtt_connect();          /* tentative de reconnexion (ENF-05) */
    }
    return link_up;
}

static int mqtt_pub(const char *topic_suffix, const uint8_t *buf, size_t len)
{
    char cmd[64];
    uint8_t c;
    uint32_t t0;
    static char line[64];
    static size_t n;
    if (!link_up) { return -1; }
    snprintf(cmd, sizeof cmd, "AT+MQTTPUBRAW=0,\"thermonode/%s/%s\",%u,0,0", NODE_ID, topic_suffix, (unsigned)len);
    if (!at_cmd(cmd, ">", 2000)) { link_up = false; return -1; }
    HAL_UART_Transmit(&huart1, buf, (uint16_t)len, 500);
    t0 = HAL_GetTick(); n = 0u;
    while ((HAL_GetTick() - t0) < 3000u) {
        if (HAL_UART_Receive(&huart1, &c, 1, 10) == HAL_OK) {
            if (n < sizeof line - 1u) { line[n++] = (char)c; line[n] = '\0'; }
            if (strstr(line, "+MQTTPUB:OK") != NULL) { return 0; }
            if (strstr(line, "FAIL") != NULL) { link_up = false; return -1; }
        }
    }
    link_up = false;
    return -1;
}

int hal_radio_send(const uint8_t *buf, size_t len)     { return mqtt_pub("data", buf, len); }
int hal_radio_send_ack(const uint8_t *buf, size_t len) { return mqtt_pub("ack", buf, len); }

/* Les messages reçus arrivent sous la forme +MQTTSUBRECV:0,"topic",len,payload ; on extrait payload */
int hal_radio_recv(uint8_t *buf, size_t cap)
{
    uint8_t c;
    while (HAL_UART_Receive(&huart1, &c, 1, 0) == HAL_OK) {
        if (rx_len < sizeof rx_buf - 1u) { rx_buf[rx_len++] = c; rx_buf[rx_len] = 0u; }
        if (c == '\n') {
            const char *p = strstr((const char *)rx_buf, "+MQTTSUBRECV:");
            if (p != NULL) {
                const char *payload = strchr(p, '{');
                if (payload != NULL) {
                    size_t n = strlen(payload);
                    while ((n > 0u) && ((payload[n - 1u] == '\r') || (payload[n - 1u] == '\n'))) { n--; }
                    if (n > cap) { n = cap; }
                    memcpy(buf, payload, n);
                    rx_len = 0u;
                    return (int)n;
                }
            }
            rx_len = 0u;
        }
    }
    return 0;
}

void hal_watchdog_kick(void) { (void)HAL_IWDG_Refresh(&hiwdg); }
