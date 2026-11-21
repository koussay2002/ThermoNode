# Rapport TP4 — Test, Validation et DevOps (ThermoNode)

## 1. Revue de code (MR !7 : fix hysteresis)
**Décision : CHANGEMENTS DEMANDÉS**
- **C1 (DR-TP4-01)** : Hystérésis en dur (`500`) au lieu de `hyst_centi` dans `threshold.c:31`. Preuve : `step(5699, true)` retourne `0` au lieu de `2`.
- **C2** : Le test couvrant l'hystérésis configurée a été désactivé (`xtest_`) et commenté. C'est pour cela que la CI reste verte malgré le bug.
- **C3 (DR-TP4-02)** : `strncpy` sans terminaison `'\0'` dans `node.c:24`. Risque de dépassement de tampon (UB).
- **C4** : Violations MISRA de style (opérateur virgule, accolades manquantes).
- **C5** : Périmètre non respecté (node.c modifié sans que ce soit annoncé).

## 2. Analyse statique C (cppcheck / MISRA)
- **Avant** : Violations de règles Required.
- **Après** : 0 violation restante. Les faux positifs (ex: règle 7.4) ont été justifiés dans `misra-suppressions.txt` avec un commentaire inline.

## 3. Analyse statique Python (pylint)
- **Avant** : Note de 9.05/10 — 18 messages (W0613, C0115, R0913, etc.)
- **Après** : Note de **10.00/10 — 0 message**.
- **Actions** : 7 commits correctifs (docstrings, arguments inutilisés, maxsplit, etc.). Les règles non pertinentes (R0913, C0103) ont été désactivées avec justification.

## 4. SonarQube
- **Quality Gate** : **Passed**
- **Fiabilité** : A (0 Bug)
- **Sécurité** : A (0 Vulnérabilité)
- **Maintenabilité** : A (0 Code Smell)
- **Duplications** : 0.0%

## 5. Git et DevOps
- **3 MRs gérées** : 
  - MR1 (fix hysteresis) : refusée (changements demandés).
  - MR2 (style/pylint) : mergée.
  - MR3 (style/misra) : mergée.
- **Conflit Git** : Provoqué sur `feature/EF-09` (deux modifications concurrentes de `THR_DEFAULT_HIGH`). Résolu avec un compromis à 6000.
- **Versioning** : Tag annoté `v0.2.0` créé et posé sur `main`.

## Réponses aux questions (Q1 à Q5)
- **Q1** : La CI était verte car le test pertinent a été renommé en `xtest_` (Unity ignore ces fonctions) et son exécution commentée. 
- **Q2** : La règle MISRA 2.5 (pas de constantes en dur) a été corrigée dans `set_period` en utilisant les macros `NODE_PERIOD_MIN_MS`.
- **Q3** : Pylint a nécessité 7 itérations. La note a baissé temporairement à cause de la normalisation sur le nombre de lignes.
- **Q4** : Le conflit Git a permis de vérifier la bonne utilisation des branches et des messages de commit conventionnels.
- **Q5** : Plan de résorption de la dette : ajout de tests unitaires manquants, configuration d'un vrai pipeline CI/CD (GitLab CI ou GitHub Actions) et ajout de l'analyse SonarQube automatique.
