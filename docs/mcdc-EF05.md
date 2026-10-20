# MC/DC sur EF-05 — Mesure invalide et reprise

## Exigence EF-05

> Mesure hors plage physique (< -40 °C ou > 125 °C) **ou** figée (5 valeurs brutes identiques consécutives) → état FAULT signalé dans le champ state ; retour à NORMAL après 5 mesures valides consécutives avec une trame de reprise.

## Décision à couvrir (fault_detect)

`fault_detect(centi, raw, valid)` renvoie `true` si :

- `D1 : !valid` (conversion ADC invalide, hors plage)
- `D2 : out_of_range(centi)` (centi < -4000 ou centi > 12500)
- `D3 : frozen(raw)` (5 valeurs brutes identiques consécutives)

La décision globale est `D1 || D2 || D3`. En pratique, `D1` et `D2` sont redondants (une valeur hors plage rend `valid=false`), mais on les traite séparément pour l'analyse MC/DC.

## Table de vérité et paires MC/DC

| Cas | D1 | D2 | D3 | Décision | Effet |
|---|---|---|---|---|---|
| T1 | F | F | F | F | mesure valide, normale |
| T2 | V | F | F | V | conversion invalide seule |
| T3 | F | V | F | V | hors plage seul (valid=true, centi hors plage) |
| T4 | F | F | V | V | capteur figé seul |
| T5 | V | V | F | V | redondance D1+D2 (vérifie que D1 seul suffit) |
| T6 | F | V | V | V | hors plage + figé |

Paires MC/DC (chaque condition modifie la décision à elle seule) :
- D1 : T1 (F) vs T2 (V) — D2, D3 fixés à F
- D2 : T1 (F) vs T3 (V) — D1, D3 fixés à F
- D3 : T1 (F) vs T4 (V) — D1, D2 fixés à F

→ 3 paires suffisent pour couvrir les 3 conditions indépendamment.

## Couverture par les tests existants

| Test | Cas MC/DC | Condition isolée |
|---|---|---|
| `test_EF05_out_of_range_enters_fault_with_frame` | T3 | D2 (hors plage) |
| `test_EF05_frozen_sensor_enters_fault` | T4 | D3 (figé) |
| `test_EF05_recovers_to_normal_after_5_valid_samples` | T1 puis reprise | D1 (valide) + reprise |
| `test_EF05_spike_isolated_is_fault_not_alert` | T3 isolé | D2 seul, sans passage en ALERT |

## Manque identifié

- Aucun test ne couvre T2 (`!valid` sans hors plage), car `temp_from_adc` renvoie toujours `valid=true` sauf si `adc > TN_ADC_MAX`. Cas non atteignable en pratique ; on le documente comme **condition non testable**.
- T5 (redondance D1+D2) n'est pas testé explicitement : le test `out_of_range` (T3) suffit, mais on pourrait ajouter un cas.

## Conclusion

- 3 paires MC/DC sur 3 conditions → couverture MC/DC **complète** pour D2 et D3, partielle pour D1 (impossible à isoler du fait de la structure de `temp_from_adc`).
- La reprise FAULT → NORMAL est couverte par `test_EF05_recovers_to_normal_after_5_valid_samples` (révèle DEF-06).
