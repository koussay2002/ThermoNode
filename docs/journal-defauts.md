# Journal des défauts révélés – TP 3

| ID | Étape | Exigence | Test révélateur | Symptôme observé (message Unity/pytest) | Hypothèse de cause | Technique qui l'a révélé |
|----|-------|----------|-----------------|------------------------------------------|--------------------|--------------------------|
| DEF-01 | 1 / 12 | EF-05 | `range_check(12500)` (limite haute) | Non révélé par les ADC entiers (12500 inatteignable : 2172→12499, 2173→12507) ; révélé à l'étape 12 par test direct de la borne | `centi < TN_TEMP_MAX_CENTI` : `<` strict rejette 12500 alors que EF-05 dit ≤ 12500 | Valeurs limites (BVA) + revue de code |
| DEF-02 | 2 | EF-03 | `test_EF03_transient_partial_window` et `test_EF03_invalid_sample_is_excluded` | `Expected 100 Was 2600` ; `Expected 2500 Was 2650` ; SAN=1 → `index 5 out of bounds for type 'int32_t [5]'` puis `global-buffer-overflow` | Débordement de tampon : la somme/l'index utilise la case [5] d'un tableau de 5 cases (off-by-one) — lecture d'une zone mémoire voisine | Valeurs limites / séquences + sanitizer |
| DEF-03 | 4 | EF-04 (règle R6) | `test_EF04_counter_resets_below_threshold` | `Expected 0 Was 1 (THR_NONE = 0, THR_RAISE = 1)` : alerte levée sur 7000,7000,2500,7000,7000 alors que seules 2 mesures consécutives sont > 6000 | Le compteur de mesures consécutives n'est pas remis à zéro au passage sous le seuil ; il persiste entre les séquences | Table de décision (règle R6) |
| DEF-04 | 6 | EI-02 (CRC) | `test_EI02_crc_check_value_123456789_is_29B1`, `test_EI02_crc_of_empty_is_FFFF` | `Expected 0x29B1 Was 0x31C3` ; `Expected 0xFFFF Was 0x0000` | L'algorithme implémenté est CRC-16/XMODEM (poly 0x1021, **init 0x0000**) et non CRC-16/CCITT-FALSE (poly 0x1021, **init 0xFFFF**) : seule la valeur initiale diffère | Oracle externe normalisé (valeur de contrôle) |
| DEF-05 | 7 | ENF-05 (tampon 60 trames) | `test_ENF05_push_61_keeps_60_and_drops_oldest` | `Expected 60 Was 61` : après 61 push, le tampon contient 61 trames au lieu de 60 ; la plus ancienne n'a pas été écrasée | Off-by-one sur la condition de saturation du tampon circulaire (`>` au lieu de `>=`, ou compteur incrémenté avant le test) — attaque « tampon N / N+1 » | Valeurs limites (BVA) |
| DEF-06 | 5 | EF-05 (transition DÉFAUT → NORMAL) | `test_EF05_recovers_to_normal_after_5_valid_samples` | `Expected 1 Was 3 (NORMAL = 1, FAULT = 3)` : après 1 pic hors plage puis 5 mesures valides, le nœud reste en FAULT | La machine à états n'implémente pas (ou mal) la transition DÉFAUT → NORMAL après 5 mesures valides consécutives ; le compteur de reprise est absent ou non remis à zéro correctement | Machine à états (table de transition) |
| DEF-07 | 8 | EI-02 (types JSON) | `test_EI02_invalid_frames_are_rejected[temp_string]`, `[seq_negative]` | `DID NOT RAISE FrameError` : `temp:"25.50"` (chaîne) accepté ; `seq:-1` (négatif) accepté | `temp = float(temp)` accepte les chaînes (conversion trop tolérante) ; aucune vérification `seq >= 0` | Valeurs limites / partitions invalides (un vecteur = une anomalie) |
| DEF-08 | 8 | ENF-05 (dédoublonnage) / EF-G1 (stockage) | `test_ENF05_duplicate_seq_is_ignored`, `test_ENF05_out_of_order_old_frame_is_ignored`, `test_last_seq_survives_restart` | `assert 0 == 1` (frames_duplicate) ; `assert 2 == 1` (out-of-order stocké) ; `(1 == 1 and 0 == 1)` (doublon non restauré au restart) | `handler.py` : la condition `if False and last is not None and f.seq <= last:` désactive tout le dédoublonnage (`if False`), donc les trames en double ou anciennes sont stockées, et `frames_duplicate` reste à 0 | Table de décision (conditions de dédoublonnage) + persistance |
| DEF-04b (supplémentaire, non listé) | 6 | EI-03 (JSON) | `test_EI03_json_get_str_rejects_unterminated_and_too_long` | `Expected FALSE Was TRUE` : `json_get_str` accepte une chaîne de 13 caractères dans un tampon de 4, ou non terminée | La fonction ne vérifie pas la taille du tampon de destination avant copie, et/ou ne vérifie pas la présence du guillemet fermant | Valeurs limites (BVA tampon) |

## Corrections (étape 12)

| ID | Fichier:ligne | Correction | Commit |
|---|---|---|---|
| DEF-01 | convert.c:5 | `<` → `<=` sur TN_TEMP_MAX_CENTI | fix(convert): accepte 125.00 °C... |
| DEF-02 | filter.c:26 | `i <= count` → `i < count` | fix(filter): corrige l'off-by-one... |
| DEF-03 | threshold.c:38 | ajout `else { above = 0u; }` | fix(threshold): remet le compteur a zero... |
| DEF-04 | frame.c:8 | `0x0000u` → `0xFFFFu` | fix(frame): init CRC-16/CCITT-FALSE... |
| DEF-04b | frame.c:87-127 | copie dans buf[256] + `n+1 >= cap` | fix(frame): json_get_str/int respectent len... |
| DEF-05 | ringbuf.c:16 | `count > RINGBUF_CAP` → `>=` | fix(ringbuf): corrige la condition de saturation... |
| DEF-06 | node.c:129 | `if (false)` → `fault_valid_streak >= 5u` + compteur | fix(node): transition FAULT->NORMAL... |
| DEF-07 | frame.py | `seq < 0` + `isinstance(temp, (int, float))` | fix(frame): types stricts... |
| DEF-08 | handler.py:32 | `if False and` → `if` | fix(handler): active le dedoublonnage... |
