# ICD — Document de contrôle des interfaces ThermoNode v1.0

## 1. Capteur → microcontrôleur (EI-01)
- Capteur TMP36 : sortie analogique 10 mV/°C, 500 mV à 0 °C, plage −40 °C … +125 °C, précision ± 2 °C (± 0,5 °C typique après calibration).
- Entrée : PA0 / ADC_IN0, 12 bits, référence 3,3 V, temps d'échantillonnage ≥ 12,5 cycles ADC (impédance de sortie du TMP36).
- Code ADC → centi-degrés : `centi = ADC * 33000 / 4096 - 5000` (entier, troncature vers zéro).
- Capteur déconnecté : entrée flottante → valeur non définie. Le firmware traite tout code hors plage physique ou figé (EF-05).

## 2. Nœud → broker MQTT (EI-02, EI-03)
| Topic | Sens | Charge utile | QoS |
|---|---|---|---|
| `thermonode/<id>/data` | nœud → passerelle | `{"id":"TN01","seq":12,"t":1700000000,"temp":25.50,"state":"NORMAL","crc":"1D0F"}` | 0 |
| `thermonode/<id>/cmd` | supervision → nœud | `{"cmd":"set_threshold","value":65,"crc":"XXXX"}` ; `cmd` ∈ {set_threshold, set_period} | 0 |
| `thermonode/<id>/ack` | nœud → supervision | `{"id":"TN01","ack":"set_threshold","result":"ok","reason":""}` ; result ∈ {ok, nack} ; reason ∈ {"", range, format, unknown} | 0 |
| `thermonode/<id>/diag` | simulateur uniquement | `{"buffered":12}` | 0 |
| `supervision/alerts` | passerelle → supervision | `{"id":"TN01","temp":70.2,"t":…}` | 0 |

- `seq` : entier croissant, 0 au démarrage ; `t` : horodatage en secondes (horloge monotone du nœud) ; `temp` : °C avec 2 décimales, signe autorisé ; `state` ∈ {INIT, NORMAL, ALERT, FAULT}.
- CRC : CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, sans réflexion, sans xorout), calculé sur les octets UTF-8 de l'objet JSON **privé du membre** `,"crc":"XXXX"` ; vecteur de contrôle : "123456789" → 0x29B1.
- Longueur maximale d'une trame : 127 octets.

## 3. Microcontrôleur ↔ module Wi-Fi ESP-01S
- USART1 115 200 bauds 8N1, PA9 (TX) → RX du module, PA10 (RX) ← TX du module ; alimentation 3,3 V (≥ 300 mA en pointe) ; niveaux 3,3 V.
- Protocole : commandes AT ESP-AT ≥ 2.2 (`AT+CWJAP`, `AT+MQTTUSERCFG`, `AT+MQTTCONN`, `AT+MQTTPUBRAW`, `AT+MQTTSUB`) ; réception `+MQTTSUBRECV:`.
