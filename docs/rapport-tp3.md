# Rapport TP3 — ThermoNode

## Réponses aux questions

### Q3 — Stub, spy ou fake dans hal_double.c

- `hal_adc_read` : stub (rejoue une séquence injectée ; joue aussi le rôle de fake car il imite le comportement d'un capteur).
- `hal_clock_ms` : fake (horloge simulée, état interne, avance contrôlée).
- `hal_radio_send` : spy (enregistre les appels et la dernière trame).
- `hal_radio_recv` : stub (file de commandes prédéfinies).

Séquence épuisée : répéter la dernière valeur plutôt que renvoyer 0, car (1) cela correspond au comportement d'un capteur stable, (2) évite de déclencher par erreur EF-05 (capteur figé), (3) rend le test déterministe.

### Q4 — Reformulation d'EF-04

Spec actuelle (docs/requirements.md, ligne 10) : « Alerte si la température filtrée est strictement supérieure au seuil haut pendant 3 mesures consécutives ; levée lorsque la température filtrée est inférieure à seuil − 5 °C ; une seule trame d'alerte émise ».

Le mot « consécutives » implique qu'une mesure non supérieure rompt la consécution : DEF-03 est donc bien un défaut, pas une ambiguïté. Deux points restent à préciser : (a) le traitement des mesures invalides, (b) l'explicitation du reset.

Reformulation proposée :

> EF-04 — Le nœud passe en ALERT si et seulement si trois mesures filtrées valides consécutives sont strictement supérieures au seuil haut configuré. Toute mesure filtrée valide inférieure ou égale au seuil haut réinitialise le compteur de consécution. Les mesures invalides sont exclues en amont par le filtre (EF-03). En ALERT, le nœud revient à NORMAL si une mesure filtrée valide est strictement inférieure à seuil_haut − hystérésis. Une valeur comprise entre seuil_haut − hystérésis (inclus) et seuil_haut (inclus) maintient ALERT. Une seule trame d'alerte est émise par entrée en ALERT.

### Q4-bis — Règle non couverte par test_threshold.c

R1 (transition d'état du nœud) n'est pas couverte ici : test_threshold.c teste le module seuil isolé, sans machine à états ni filtre. C'est normal : R1 relève du niveau nœud et est couverte à l'étape 5 (test_node.c).

### Q6 — Table d'états et transitions

États : INIT, NORMAL, ALERT, FAULT.
Événements : init_ok, 3 filtrées > seuil, < seuil − hyst, mesure invalide, 5 valides consécutives, commande set_threshold.

| État \ Événement | init_ok | 3 filtrées > seuil | < seuil − hyst | mesure invalide | 5 valides consécutives | set_threshold |
|---|---|---|---|---|---|---|
| INIT | → NORMAL | — | — | → FAULT (implicite) | — | ack |
| NORMAL | — | → ALERT | — | → FAULT | — | ack ok/range/CRC |
| ALERT | — | reste ALERT | → NORMAL (borne testée) | → FAULT (non testé) | — | — |
| FAULT | — | — | — | reste FAULT | → NORMAL (test EF05_recovers, révèle DEF-06) | ack (non testé) |

Transitions couvertes : INIT→NORMAL, NORMAL→ALERT (3 tests EF-04), ALERT→NORMAL (dont borne d'hystérésis), NORMAL→FAULT (hors plage + capteur figé), commandes EF-07/EF-08/EI-03 (ok/range/format/CRC).
Transition non testée : ALERT → FAULT (mesure invalide pendant une alerte).
Événement non spécifié : FAULT → ALERT (faut-il repasser en NORMAL d'abord, ou aller directement en ALERT si les 5 mesures valides sont elles-mêmes > seuil ?). À préciser dans requirements.md.
