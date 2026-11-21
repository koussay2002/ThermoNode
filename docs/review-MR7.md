# Revue de la MR !7 — fix hysteresis (A. Dupont)

**Branche revue** : fix/EF-04-hysteresis (commit 3319519)
**Base** : main (d5ecfc4)
**Fichiers touchés** : 3 (+9 / -15)
**Décision** : CHANGEMENTS DEMANDÉS

## Constats

### C1 — [BLOQUANT] Hystérésis en dur (DR-TP4-01)
firmware/src/threshold.c:31
    if (filtered_centi < high_centi - 500)
La constante 500 remplace hyst_centi. threshold_set(6000, 300) puis threshold_step(5699, true) retourne 0 (THR_NONE) au lieu de 2 (THR_CLEAR).

### C2 — [BLOQUANT] Test révélateur désactivé (DR-TP4-01)
firmware/test/test_threshold.c:55 et :84
Le test a été renommé en xtest_ et son RUN_TEST commenté (flaky). La CI reste verte (6 suites, 0 Failures).

### C3 — [BLOQUANT] strncpy sans terminaison (DR-TP4-02)
firmware/src/node.c:24 : strncpy(node_id, id, TN_ID_LEN);
Le patch supprime node_id[TN_ID_LEN - 1u] = '\0'; → si strlen(id) >= TN_ID_LEN, node_id n'est pas terminé (UB).

### C4 — [MAJEUR] Violations MISRA style
Opérateur virgule (ev = THR_CLEAR, above = 0u), accolades manquantes sur if/else.

### C5 — [MAJEUR] Périmètre et forme du commit
node.c touché hors périmètre, refactorisation non annoncée, message non conforme.

## Réponse à Q1
La CI est verte car le seul test couvrant l'hystérésis configurée est renommé en xtest_ et son RUN_TEST commenté. Unity ignore les xtest_*, donc le test ne s'exécute pas. 0 Failures mais couverture réelle nulle.

## Décision : CHANGEMENTS DEMANDÉS
1. Remplacer 500 par hyst_centi (threshold.c:31)
2. Réactiver le test (retirer x, décommenter RUN_TEST)
3. Restaurer la terminaison '\0' (node.c:24)
4. Remettre accolades, supprimer opérateur virgule
5. Sortir node.c de la MR ou l'annoncer

## DR-TP4-01
Défaut : hystérésis en dur à 500 au lieu de hyst_centi
Localisation : firmware/src/threshold.c:31
Preuve : threshold_set(6000, 300) → step(5699, true) = 0 (attendu 2)
Correction : if (filtered_centi < high_centi - hyst_centi)

## DR-TP4-02
Défaut : strncpy sans terminaison '\0'
Localisation : firmware/src/node.c:24
Preuve : lecture de code, la ligne node_id[TN_ID_LEN-1] = '\0' a été retirée
Impact : si strlen(id) >= TN_ID_LEN, node_id n'est pas terminé (UB)
Correction : restaurer la terminaison explicite
