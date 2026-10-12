/* frame.h — EI-02 trames JSON + CRC-16/CCITT-FALSE ; EI-03 commandes et acquittements */
#ifndef TN_FRAME_H
#define TN_FRAME_H
#include "types.h"

uint16_t crc16_ccitt_false(const uint8_t *data, size_t len);
/* Construit {"id":..,"seq":..,"t":..,"temp":..,"state":"..","crc":"XXXX"} ; le CRC est calculé sur
 * l'objet JSON sans le membre crc. Renvoie la longueur écrite (sans '\0') ou 0 si cap insuffisante. */
size_t   frame_build(uint8_t *out, size_t cap, const frame_t *f);
const char *state_name(node_state_t s);
/* Vérifie le CRC d'un texte JSON (terminé par '\0') contenant ,"crc":"XXXX" ; true si valide */
bool     frame_crc_ok(const uint8_t *json, size_t len);
/* Extrait un entier suivant la clé "key": ; false si absent */
bool     json_get_int(const uint8_t *json, size_t len, const char *key, int32_t *value);
/* Extrait une chaîne suivant la clé "key":" dans out (cap) ; false si absente */
bool     json_get_str(const uint8_t *json, size_t len, const char *key, char *out, size_t cap);

#endif
