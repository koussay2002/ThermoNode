# Référentiel d'exigences ThermoNode — v1.0

Format exploité par `tools/traceability.py` : `| ID | Prio | Titre | Énoncé | Catégorie |`.

| ID | Prio | Titre | Énoncé | Catégorie |
|---|---|---|---|---|
| EF-01 | P1 | Période d'acquisition | Le nœud acquiert la température avec une période configurable de 1 à 60 s, 1 000 ms ± 50 ms par défaut | Fonctionnel + temporel |
| EF-02 | P1 | Conversion ADC | T(°C) = (ADC × 3,3 / 4096 − 0,5) × 100 en centi-degrés (arithmétique entière) ; un code ADC > 4095 est invalide | Fonctionnel |
| EF-03 | P2 | Filtrage | La température filtrée est la moyenne glissante des 5 dernières mesures valides | Fonctionnel |
| EF-04 | P1 | Alerte seuil haut | Alerte si la température filtrée est strictement supérieure au seuil haut (60 °C par défaut) pendant 3 mesures consécutives ; levée lorsque la température filtrée est inférieure à seuil − 5 °C (hystérésis) ; une seule trame d'alerte émise | Fonctionnel / sûreté |
| EF-05 | P1 | Mesure invalide | Mesure hors plage physique (< −40 °C ou > 125 °C) ou figée (5 valeurs brutes identiques consécutives) → état DÉFAUT signalé dans le champ state ; retour à NORMAL après 5 mesures valides consécutives avec une trame de reprise | Tolérance aux fautes |
| EF-06 | P2 | Trame périodique | Une trame de données est publiée toutes les 10 s ; une trame d'alerte est publiée immédiatement | Fonctionnel |
| EF-07 | P2 | Commande de seuil | Commande set_threshold acceptée entre 20 et 100 °C ; hors plage → nack "range" ; commande au CRC invalide ignorée sans acquittement | Fonctionnel |
| EF-08 | P3 | Commande de période | Commande set_period acceptée entre 1 et 60 s | Fonctionnel |
| ENF-01 | P1 | Latence d'alerte | Latence entre la 3ᵉ mesure filtrée dépassant le seuil et la publication de la trame d'alerte < 2 s | Performance (temporel) |
| ENF-02 | P2 | Consommation | Consommation moyenne < 5 mA sous 3,3 V sur 1 h de fonctionnement normal (mesure sur banc) | Performance (ressources) |
| ENF-03 | P1 | Empreinte mémoire | Modules applicatifs : flash < 48 Ko, RAM < 8 Ko (rapport size, porte CI) | Performance (ressources) |
| ENF-04 | P1 | Règles de codage | C99, MISRA C:2012 (règles obligatoires et requises) ; dérogations documentées dans docs/misra-deviations.md | Maintenabilité |
| ENF-05 | P2 | Perte de liaison | En cas de perte de liaison, mémorise jusqu'à 60 trames (la plus ancienne est écrasée) et les retransmet dans l'ordre à la reconnexion ; la passerelle ignore les doublons | Fiabilité (récupération) |
| ENF-06 | P2 | Robustesse réseau | Avec 30 % de trames perdues, le nœud ne se bloque pas et reprend l'émission normale en moins de 30 s après rétablissement | Fiabilité |
| EI-01 | P1 | Interface capteur | Capteur TMP36 (10 mV/°C, offset 500 mV) sur entrée ADC 12 bits 0–3,3 V | Interface matérielle |
| EI-02 | P1 | Format des trames | JSON {id, seq, t, temp, state, crc} sur thermonode/<id>/data ; crc = CRC-16/CCITT-FALSE (hex 4) de l'objet JSON sans le membre crc ; temp en °C à 2 décimales | Interface protocole |
| EI-03 | P2 | Commandes | Commandes JSON {cmd, value, crc} sur thermonode/<id>/cmd ; acquittement {id, ack, result, reason} sur thermonode/<id>/ack en moins de 1 s | Interface protocole |
| C-01 | P1 | Cible | Cortex-M0+ (STM32G031K8), 64 Ko flash, 8 Ko RAM | Contrainte matérielle |
| C-02 | P1 | Testabilité | C99 compilé avec GCC ; logique applicative testable hors cible via une couche HAL | Contrainte technique |
| C-03 | P2 | Passerelle | Passerelle Python 3.12 : stockage SQLite 90 jours (EF-G1), rejet et journalisation des trames au CRC invalide (EF-G2), dédoublonnage par numéro de séquence | Contrainte technique |
