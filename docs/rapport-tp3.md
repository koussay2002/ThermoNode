# Rapport TP3 — ThermoNode

## Réponses aux questions

### Q3 — Stub, spy ou fake dans hal_double.c

- `hal_adc_read` : stub (rejoue une séquence injectée ; joue aussi le rôle de fake car il imite le comportement d'un capteur).
- `hal_clock_ms` : fake (horloge simulée, état interne, avance contrôlée).
- `hal_radio_send` : spy (enregistre les appels et la dernière trame).
- `hal_radio_recv` : stub (file de commandes prédéfinies).

Séquence épuisée : répéter la dernière valeur plutôt que renvoyer 0, car (1) cela correspond au comportement d'un capteur stable, (2) évite de déclencher par erreur EF-05 (capteur figé), (3) rend le test déterministe.
