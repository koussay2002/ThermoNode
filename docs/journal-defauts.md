# Journal des défauts révélés – TP 3

| ID | Étape | Exigence | Test révélateur | Symptôme observé (message Unity/pytest) | Hypothèse de cause | Technique qui l'a révélé |
|----|-------|----------|-----------------|------------------------------------------|--------------------|--------------------------|
| DEF-01 | 1 / 12 | EF-05 | `range_check(12500)` (limite haute) | Non révélé par les ADC entiers (12500 inatteignable : 2172→12499, 2173→12507) ; révélé à l'étape 12 par test direct de la borne | `centi < TN_TEMP_MAX_CENTI` : `<` strict rejette 12500 alors que EF-05 dit ≤ 12500 | Valeurs limites (BVA) + revue de code |
| DEF-02 | 2 | EF-03 | `test_EF03_transient_partial_window` et `test_EF03_invalid_sample_is_excluded` | `Expected 100 Was 2600` ; `Expected 2500 Was 2650` ; SAN=1 → `index 5 out of bounds for type 'int32_t [5]'` puis `global-buffer-overflow` | Débordement de tampon : la somme/l'index utilise la case [5] d'un tableau de 5 cases (off-by-one) — lecture d'une zone mémoire voisine | Valeurs limites / séquences + sanitizer |
| DEF-03 | 4 | EF-04 (règle R6) | `test_EF04_counter_resets_below_threshold` | `Expected 0 Was 1 (THR_NONE = 0, THR_RAISE = 1)` : alerte levée sur 7000,7000,2500,7000,7000 alors que seules 2 mesures consécutives sont > 6000 | Le compteur de mesures consécutives n'est pas remis à zéro au passage sous le seuil ; il persiste entre les séquences | Table de décision (règle R6) |
| DEF-04 | | | | | | |
| DEF-05 | | | | | | |
| DEF-06 | | | | | | |
| DEF-07 | | | | | | |
| DEF-08 | | | | | | |
