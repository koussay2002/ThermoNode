#include <errno.h>
#include "frame.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint16_t crc16_ccitt_false(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    size_t i;
    int b;
    for (i = 0u; i < len; i++) {
        crc ^= (uint16_t)((unsigned int)data[i] << 8);
        for (b = 0; b < 8; b++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)(((unsigned int)crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)((unsigned int)crc << 1);
            }
        }
    }
    return crc;
}

const char *state_name(node_state_t s)
{
    switch (s) {
        case STATE_NORMAL: return "NORMAL";
        case STATE_ALERT:  return "ALERT";
        case STATE_FAULT:  return "FAULT";
        case STATE_INIT:
        default:           return "INIT";
    }
}

size_t frame_build(uint8_t *out, size_t cap, const frame_t *f)
{
    char payload[TN_FRAME_MAX];
    int32_t t = f->temp_centi;
    const char *sign = (t < 0) ? "-" : "";
    int32_t a = (t < 0) ? -t : t;
    int n;
    uint16_t crc;

    int32_t whole = a / 100;
    int32_t frac = a % 100;

    n = snprintf(payload, sizeof payload,
                 "{\"id\":\"%s\",\"seq\":%lu,\"t\":%lu,\"temp\":%s%ld.%02ld,\"state\":\"%s\"}",
                 f->id, (unsigned long)f->seq, (unsigned long)f->t,
                 sign, (long)whole, (long)frac, state_name(f->state));
    if ((n <= 0) || ((size_t)n >= sizeof payload)) {
        return 0u;
    }
    crc = crc16_ccitt_false((const uint8_t *)payload, (size_t)n);
    /* remplace le '}' final par ,"crc":"XXXX"} */
    if (cap < (size_t)n + 14u + 1u) {
        return 0u;
    }
    (void)memcpy(out, payload, (size_t)n - 1u);
    n = snprintf((char *)out + (n - 1), cap - ((size_t)n - 1u), ",\"crc\":\"%04X\"}", crc) + (n - 1);
    return (size_t)n;
}

bool frame_crc_ok(const uint8_t *json, size_t len)
{
    static const char key[] = ",\"crc\":\"";
    uint8_t tmp[TN_FRAME_MAX];
    const uint8_t *p;
    size_t pos;
    size_t hexlen = 4u;
    size_t rest;
    uint16_t given;
    uint16_t calc;
    if (len > sizeof tmp) {
        return false;
    }
    p = (const uint8_t *)strstr((const char *)json, key);
    if (p == NULL) {
        return false;
    }
    ptrdiff_t off = p - json;
    pos = (size_t)off;
    if ((pos + (sizeof(key) - 1u) + hexlen + 1u) > len) {
        return false;
    }
    given = (uint16_t)strtoul((const char *)p + sizeof(key) - 1u, NULL, 16);
    rest = len - (pos + sizeof(key) - 1u + hexlen + 1u);
    (void)memcpy(tmp, json, pos);
    (void)memcpy(tmp + pos, json + pos + sizeof(key) - 1u + hexlen + 1u, rest);
    calc = crc16_ccitt_false(tmp, pos + rest);
    return calc == given;
}

bool json_get_int(const uint8_t *json, size_t len, const char *key, int32_t *value)
{
    char pat[32];
    char buf[256];
    const char *p;
    if (len >= sizeof buf) {
        return false;
    }
    (void)memcpy(buf, json, len);
    buf[len] = '\0';
    (void)snprintf(pat, sizeof pat, "\"%s\":", key);
    p = strstr(buf, pat);
    if (p == NULL) {
        return false;
    }
    p += strlen(pat);
    if ((*p != '-') && ((*p < '0') || (*p > '9'))) {
        return false;
    }
    errno = 0;
    *value = (int32_t)strtol(p, NULL, 10);
    return true;
}
bool json_get_str(const uint8_t *json, size_t len, const char *key, char *out, size_t cap)
{
    char pat[32];
    char buf[256];
    const char *p;
    const char *e;
    size_t n;
    if (len >= sizeof buf) {
        return false;
    }
    (void)memcpy(buf, json, len);
    buf[len] = '\0';
    (void)snprintf(pat, sizeof pat, "\"%s\":\"", key);
    p = strstr(buf, pat);
    if (p == NULL) {
        return false;
    }
    p += strlen(pat);
    e = strchr(p, '"');
    if (e == NULL) {
        return false;
    }
    ptrdiff_t span = e - p;
    n = (size_t)span;
    if ((n + 1u) >= cap) {        /* au lieu de n + 1u > cap : pas de place pour '\0' si n+1 >= cap */
        return false;
    }
    (void)memcpy(out, p, n);
    out[n] = '\0';
    return true;
}
