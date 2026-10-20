# Rapport TP3 — ThermoNode

## 1. Techniques appliquées par exigence

| Exigence | Technique | Fichier de test | Nombre de cas |
|---|---|---|---|
| EF-02 (conversion) | Partitions d'équivalence + BVA | firmware/test/test_convert.c | 6 |
| EF-03 (filtre) | Séquences (fenêtre vide/partielle/pleine/glissement) | firmware/test/test_filter.c | 6 |
| EF-04 (seuil) | Table de décision (R1–R6) + BVA | firmware/test/test_threshold.c | 9 |
| EF-05 (mesure invalide) | BVA (bornes ADC) + machine à états + MC/DC | test_convert.c, test_node.c, mcdc-EF05.md | 6 + 3 |
| EF-06 (trame périodique) | Horloge simulée (fake) | test_node.c | 1 |
| EF-07 (commande seuil) | Partitions (ok/range/CRC invalide) + BVA 19/20/100/101 | test_threshold.c, test_node.c | 4 |
| EF-08 (commande période) | Partitions (ok/range) + BVA 1/60/61 | test_node.c | 2 |
| ENF-01 (latence alerte) | Horloge simulée (fake) | test_node.c | 1 |
| ENF-05 (tampon + doublons) | BVA 0/1/59/60/61 + table de décision + intégration SIL | test_ringbuf.c, test_handler.py, test_reconnect.py | 4 + 8 + 1 |
| EI-02 (CRC) | Oracle externe normalisé (0x29B1, 0xFFFF) | test_frame.c, test_frame.py | 9 + 4 |
| EI-03 (JSON) | BVA tampon + partitions invalides | test_frame.c, test_frame.py | 2 |
| EI-01 (ADC) | Codes impossibles (4096, 65535) — non testable fonctionnellement | — | 0 |

## 2. Journal des défauts

Le journal complet (ID, étape, exigence, test révélateur, symptôme, cause, technique, correction, commit) est dans `docs/journal-defauts.md`. **Neuf défauts** ont été révélés : DEF-01 à DEF-08 (annoncés par le guide) et DEF-04b (supplémentaire, révélé par `test_EI03_json_get_str_rejects_unterminated_and_too_long`).

Chaque défaut a été corrigé par une branche dédiée `fix/DEF-xx-...`, avec un commit `fix(<module>): <quoi> (DEF-xx, <exigence>)` et un merge `--no-ff`.

## 3. Réponses aux questions Q1–Q11

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

## Limites de la suite

- **ENF-05 système** : le test d'intégration est marqué `xfail` car `hal_sim.c` ne relit pas les lignes ajoutées à son fichier scénario après EOF (`clearerr` + `fseek` insuffisants sur glibc). La logique ENF-05 est validée au niveau unitaire par `test_ENF05_*` (firmware) et `test_ENF05_*` (gateway), qui passent.
- **DEF-05 / DEF-08 non révélés au niveau système** (cf. Q11) : le test n'injecte que 30 mesures (< 60 pour le ringbuf) et ne redémarre pas le nœud, donc ni le débordement de tampon ni les doublons ne sont déclenchés au niveau système. Ils sont révélés au niveau unitaire.

---

## 4. Couverture et ressources

| Moment | Lignes | Branches | Flash | RAM |
|---|---|---|---|---|
| TP 0 (départ) | ~5 % | ~5 % | — | — |
| Avant correction (étape 11) | 89,5 % (246/275) | 86,0 % (117/136) | 10 148 / 49 152 (20,6 %) | 1 640 / 8 192 (20,0 %) |
| Après correction | 90,8 % (268/295) | 85,8 % (127/148) | OK | OK |

- **Firmware** : 61 tests Unity, 0 échec (6+6+11+25+4+9).
- **Gateway** : 22 tests unit, 0 échec ; 1 test d'intégration xfailed (limite du simulateur).
- **SAN=1** : 0 ligne AddressSanitizer.
- **Branches restées rouges** : la branche `if (false)` de `node.c` (DEF-06) était du code mort non couvert avant correction. Après correction, la transition FAULT→NORMAL est couverte par `test_EF05_recovers_to_normal_after_5_valid_samples`. Les branches restantes concernent des états dégradés (commande sans `value`, CRC partiel) couverts par les tests mais sous-comptés par la couverture de branches.

## 5. Tests CPS (cyber-physiques)

- **Temps (ENF-01)** : `test_ENF01_alert_frame_within_2s_of_third_filtered_sample` utilise une horloge simulée (`fake_ms` avancé par `hal_double_advance_ms`). L'oracle vérifie `last_send_ms < t3 + 2000`, c'est-à-dire que la trame d'alerte est émise moins de 2 s après la 3e mesure filtrée > seuil. Aucun `sleep` réel : la contrainte temporelle est testée de façon déterministe.
- **Communication (ENF-05)** : le test unitaire `test_ENF05_frames_buffered_and_resent_in_order_after_outage` vérifie qu'après une coupure simulée, 11 trames sont tamponnées puis retransmises dans l'ordre. Le test système `test_reconnect.py` (xfail) visait la même propriété au niveau SIL Docker Compose ; il est bloqué par une limite du simulateur (voir section 6).
- **Capteur (EF-05)** : `test_EF05_out_of_range_enters_fault_with_frame` (129,99 °C → FAULT), `test_EF05_frozen_sensor_enters_fault` (5 valeurs brutes identiques → FAULT), `test_EF05_spike_isolated_is_fault_not_alert` (pic isolé → FAULT, pas ALERT). La reprise après 5 valides est couverte par `test_EF05_recovers_to_normal_after_5_valid_samples`.
- **Ressources** : `make -C firmware size-check` vérifie flash ≤ 49 152 octets et RAM ≤ 8 192 octets (flash 20,6 %, RAM 20,0 %). `make -C firmware test SAN=1` (AddressSanitizer) détecte les débordements mémoire et est vert (0 ligne ASan) après correction.

## 6. Limites de la suite et temps passé

### Limites
- **ENF-05 système (xfail)** : `hal_sim.c` ne relit pas correctement les lignes ajoutées à son fichier scénario après EOF (`clearerr` + `fseek` insuffisants sur glibc). Le test d'intégration ne peut pas injecter `CONN:0` / `CONN:1` à la volée. La logique ENF-05 est validée au niveau unitaire.
- **DEF-05 et DEF-08 non révélés au niveau système** (cf. Q11) : le test n'injecte que 30 mesures (< 60 pour le ringbuf) et ne redémarre pas le nœud. Ils sont révélés au niveau unitaire, ce qui est préférable (plus rapide, plus déterministe, cause isolée).
- **EI-01 (ADC 4096 / 65535)** : non testable au niveau fonctionnel, car `hal_adc_read` renvoie `uint16_t` et le stub ne permet pas de pousser 65535 sans modifier le type. Documenté comme non testable.

### Temps passé par étape
| Étape | Temps |
|---|---|
| 0 (point de départ) | 10 min |
| 1 (partitions convert) | 40 min |
| 2 (filtre, ASan) | 40 min |
| 3 (doublures HAL) | 20 min |
| 4 (table de décision) | 30 min |
| 5 (machine à états) | 55 min |
| 6 (CRC, JSON) | 25 min |
| 7 (ringbuf) | 20 min |
| 8 (gateway unit) | 35 min |
| 9-10 (intégration) | 60 min |
| 11 (couverture) | 15 min |
| 12 (corrections) | 90 min |
| Rapport et debug | 120 min |
| **Total** | **~9 h 20** |
