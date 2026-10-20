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

### Q1 — Valeurs calculées à la main (tableau étape 1.1)

| ADC | Calcul (ADC×33000/4096 − 5000) | centi | Valide ? | Partition / limite |
|---:|---|---:|:---:|---|
| 0 | 0 − 5000 | −5000 | non | invalide basse (−50 °C) |
| 124 | 124×33000/4096 = 999 ; 999−5000 | −4001 | non | juste sous −40 °C |
| 125 | 125×33000/4096 = 1007 ; 1007−5000 | −3993 | oui | 1re valeur valide |
| 931 | 931×33000/4096 = 7500 ; 7500−5000 | 2500 | oui | nominal 25 °C |
| 2048 | 2048×33000/4096 = 16500 ; 16500−5000 | 11500 | oui | médiane de l'échelle |
| 2172 | 2172×33000/4096 = 17499 ; 17499−5000 | 12499 | oui | dernière valeur valide (124,99 °C) |
| 2173 | 2173×33000/4096 = 17507 ; 17507−5000 | 12507 | non | juste au-dessus de 125 °C |
| 4095 | 4095×33000/4096 = 32991 ; 32991−5000 | 27991 | non | borne ADC 12 bits |
| 4096 | impossible (ADC > TN_ADC_MAX) | — | — | code impossible (EI-01) |
| 65535 | impossible (uint16_t hors ADC 12 bits) | — | — | limite du type |

Ces valeurs confirment le cours : 4095 → 27991 ; 2173 → 12507 ; 2172 → 12499. L'oracle est calculé **avant** de lire le code.

### Q2 — Expected 100 Was 2600 : d'où vient le 2600 ?

Le filtre fait une moyenne glissante sur 5 cases (`int32_t window[5]`). Le bug (DEF-02) est la boucle `for (i = 0; i <= count; i++)` : quand `count == 1` (première mesure valide), `i` va de 0 à 1, donc on lit `window[1]` qui n'a pas encore été écrit (valeur résiduelle 2500, laissée par un test précédent ou par l'initialisation). La somme vaut `100 + 2500 = 2600`, divisée par `count = 1` → 2600. Aucune valeur 2600 n'a été poussée : c'est une lecture mémoire voisine.

**Pourquoi le régime établi passe ?** Dans `steady_state_five_identical`, toutes les valeurs sont 2500 et la case [1] (ou [5]) contient aussi 2500 — le bug est invisible.

**Le sanitizer est un oracle différent** : l'assertion Unity juge la **valeur** (100 attendu, 2600 obtenu) ; ASan juge la **mémoire** (`index 5 out of bounds for type 'int32_t [5]'`). ASan identifie la **cause** (off-by-one), l'assertion identifie le **symptôme**.

### Q3 — Stub, spy ou fake dans hal_double.c

- `hal_adc_read` : **stub** (rejoue une séquence injectée ; joue aussi le rôle de fake car il imite un capteur).
- `hal_clock_ms` : **fake** (horloge simulée, état interne, avance contrôlée).
- `hal_radio_send` : **spy** (enregistre les appels et la dernière trame).
- `hal_radio_recv` : **stub** (file de commandes prédéfinies).

**Séquence épuisée → répéter la dernière valeur** : (1) c'est le comportement d'un capteur stable, (2) évite de déclencher par erreur EF-05 (capteur figé), (3) rend le test déterministe.

### Q4 — Reformulation d'EF-04

Spec actuelle (ligne 10 de `docs/requirements.md`) : « Alerte si la température filtrée est strictement supérieure au seuil haut pendant 3 mesures consécutives ; levée lorsque la température filtrée est inférieure à seuil − 5 °C ; une seule trame d'alerte émise ».

Le mot « consécutives » implique qu'une mesure non supérieure rompt la consécution : DEF-03 est donc bien un défaut, pas une ambiguïté. Deux points restent à préciser : (a) le traitement des mesures invalides, (b) l'explicitation du reset.

**Reformulation proposée :**

> EF-04 — Le nœud passe en ALERT si et seulement si **trois mesures filtrées valides consécutives** sont **strictement supérieures** au seuil haut configuré. Toute mesure filtrée valide **inférieure ou égale** au seuil haut **réinitialise** le compteur de consécution. Les mesures invalides sont exclues en amont par le filtre (EF-03). En ALERT, le nœud revient à NORMAL si une mesure filtrée valide est **strictement inférieure** à `seuil_haut − hystérésis`. Une valeur comprise entre `seuil_haut − hystérésis` (inclus) et `seuil_haut` (inclus) maintient ALERT. Une seule trame d'alerte est émise par entrée en ALERT.

### Q4-bis — Règle non couverte par test_threshold.c

R1 (transition d'état du nœud) n'est pas couverte ici : `test_threshold.c` teste le module seuil isolé, sans machine à états ni filtre. C'est normal : R1 relève du niveau nœud et est couverte à l'étape 5 (`test_node.c`).

### Q5 — Test qui échoue « pour une mauvaise raison »

Le test `test_EF04_alert_after_three_above` avec `feed(ADC_70, 3)` échoue (`Expected 2 Was 1`). Ce n'est pas un défaut du code, c'est une erreur de conception du test.

Calcul à la main de la moyenne glissante (5 mesures à 25 °C puis n mesures à 70 °C) : 34, 43, 52, **61**, **70**, **70**. Il faut donc **6 mesures brutes à 70 °C**, pas 3, pour que 3 mesures filtrées consécutives dépassent 60.

Si on avait utilisé `ADC_130` (hors plage) « pour être sûr de dépasser », le nœud serait passé en **FAULT** (EF-05), pas en ALERT, et le test aurait échoué pour une autre raison.

**Règle de méthode** : (1) calculer l'oracle à la main avant d'écrire le test ; (2) n'activer qu'une seule condition à la fois ; (3) distinguer « test faux » et « code faux » avant de corriger.

### Q6 — Table d'états et transitions

États : INIT, NORMAL, ALERT, FAULT. Événements : init_ok, 3 filtrées > seuil, < seuil − hyst, mesure invalide, 5 valides consécutives, commande set_threshold.

| État \ Événement | init_ok | 3 filtrées > seuil | < seuil − hyst | mesure invalide | 5 valides | set_threshold |
|---|---|---|---|---|---|---|
| INIT | → NORMAL | — | — | → FAULT | — | ack |
| NORMAL | — | → ALERT | — | → FAULT | — | ack |
| ALERT | — | reste ALERT | → NORMAL | → FAULT (non testé) | — | — |
| FAULT | — | — | — | reste FAULT | → NORMAL | ack |

Transitions couvertes : INIT→NORMAL, NORMAL→ALERT (3 tests EF-04), ALERT→NORMAL (dont borne d'hystérésis), NORMAL→FAULT (hors plage + figé), commandes EF-07/EF-08/EI-03.

Transition non testée : **ALERT → FAULT** (mesure invalide pendant une alerte). Événement non spécifié : **FAULT → ALERT** (faut-il repasser par NORMAL d'abord ?). À préciser dans `requirements.md`.

### Q7 — CRC-16/XMODEM vs CCITT-FALSE

`0x31C3` est la valeur de contrôle de **CRC-16/XMODEM** (poly 0x1021, init **0x0000**, xorout 0x0000). Pour **CRC-16/CCITT-FALSE** (poly 0x1021, init **0xFFFF**, xorout 0x0000), la valeur est `0x29B1`. La seule différence de paramètre est la **valeur initiale** : 0x0000 vs 0xFFFF.

**Pourquoi `frame_has_all_fields_and_valid_crc` passe malgré DEF-04 ?** Parce que `frame_build` (émetteur) et `frame_crc_ok` (vérificateur) utilisent **le même** CRC défectueux : ils sont cohérents entre eux. Seul un **oracle externe normalisé** (0x29B1) révèle le défaut.

**TP 0 (`gateway_rejected_crc = 100 %`)** : la gateway Python (CCITT-FALSE) rejetait les trames du firmware C (XMODEM). C'est **le firmware qui avait tort** (DEF-04). L'oracle qui tranche est la valeur normalisée 0x29B1.

### Q8 — Pourquoi l'oracle externe du CRC est-il indépendant du code ?

Un CRC n'est pas « ce que renvoie la fonction » mais une **valeur de contrôle normalisée** par une spec (poly, init, refin, refout, xorout). Pour CRC-16/CCITT-FALSE, `crc("123456789") = 0x29B1` et `crc("") = 0xFFFF`. Ces valeurs sont publiées et **ne dépendent pas** de l'implémentation. C'est un oracle **externe** : si `frame_build` et `frame_crc_ok` utilisent le même CRC « faux », les deux sont cohérents entre eux et les tests `frame_has_all_fields_and_valid_crc` / `corrupted_frame_fails_crc` passent — mais l'oracle normalisé révèle DEF-04. C'est le principe de la diapo 5 : l'oracle ne doit pas être calculé par le code testé.

### Q9 — Lien avec la « surprise » du TP 0

Au TP 0, la gateway Python (CCITT-FALSE, init 0xFFFF) rejetait **100 %** des trames du firmware C (XMODEM, init 0x0000). C'était **le firmware qui avait tort** (DEF-04). L'oracle qui a tranché est la valeur normalisée `0x29B1` : le code C produisait `0x31C3`, valeur de XMODEM, pas de CCITT-FALSE. Après correction de l'init à `0xFFFF`, les deux programmes sont alignés et les trames ne sont plus rejetées.

### Q10 — Une seule trame cumulant plusieurs anomalies

Si on avait écrit un **seul** vecteur cumulant `temp:"25.50"` (chaîne) + `seq:-1` (négatif) + CRC faux, `pytest.raises(FrameError)` aurait été satisfait par la **première anomalie détectée** (le CRC), et le test aurait **passé au vert**. On aurait conclu à tort que `parse_frame` rejette **toutes** les anomalies, alors que `temp_string` et `seq_negative` sont tolérées. Le diagnostic serait impossible.

**Règle** : un cas de test = **une seule anomalie**, avec un `id=` lisible (diapo 8). C'est ce que fait `pytest.mark.parametrize` du guide.

### Q11 — Pourquoi le test système ne révèle-t-il ni DEF-05 ni DEF-08 ?

Le test n'injecte que **30 mesures** (< 60 pour le ringbuf) → le tampon n'atteint jamais sa limite → DEF-05 invisible. Il ne **redémarre pas le nœud** ni n'envoie de doublons → DEF-08 invisible.

**Pour les révéler au niveau système** : (a) injecter **61 mesures** pendant la coupure (révèle DEF-05 : la 61e doit être écrasée) ; (b) **redémarrer le nœud** après une coupure et vérifier que la gateway ignore les séquences déjà vues (révèle DEF-08).

**Pourquoi préfère-t-on les révéler au niveau unitaire ?** Plus rapide, plus déterministe, cause isolée. Un test système qui échoue est coûteux à diagnostiquer et peut être instable.

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
