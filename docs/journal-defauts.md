# Journal des défauts révélés – TP 3

| ID | Étape | Exigence | Test révélateur | Symptôme observé (message Unity/pytest) | Hypothèse de cause | Technique qui l'a révélé |
|----|-------|----------|-----------------|------------------------------------------|--------------------|--------------------------|
| DEF-01 | 1 / 12 | EF-05 | `range_check(12500)` (limite haute) | Non révélé par les ADC entiers (12500 inatteignable : 2172→12499, 2173→12507) ; révélé à l'étape 12 par test direct de la borne | `centi < TN_TEMP_MAX_CENTI` : `<` strict rejette 12500 alors que EF-05 dit ≤ 12500 | Valeurs limites (BVA) + revue de code |
| DEF-02 | | | | | | |
| DEF-03 | | | | | | |
| DEF-04 | | | | | | |
| DEF-05 | | | | | | |
| DEF-06 | | | | | | |
| DEF-07 | | | | | | |
| DEF-08 | | | | | | |
