# Revue de MR !7 « fix hysteresis » — réviseurs : <A>, <B>

Branche `fix/EF-04-hysteresis` → `main` — auteur : A. Dupont — commit `1e6862c`
Tests : 6 suites vertes, 0 échec (la suite `test_threshold` passe de 9 à 8 tests : le test désactivé a disparu), cppcheck « classique » : rien. **Décision : demander des changements.**

| # | Fichier:ligne | Constat | Règle / source | Gravité | Demande à l'auteur |
|---|---|---|---|---|---|
| 1 | firmware/src/threshold.c:31 (`threshold_step`) | `high_centi - 500` : l'hystérésis configurée (`hyst_centi`) est remplacée par une constante en dur. Régression : `threshold_set(6000, 300)` puis `threshold_step(5699, true)` renvoie `THR_NONE` au lieu de `THR_CLEAR`. La valeur 500 ne coïncide avec le comportement attendu que pour l'hystérésis par défaut. | EF-04 (« seuil − hystérésis »), liste de contrôle « constantes / constante configurée ignorée » | **Bloquant** | Revenir à `(high_centi - hyst_centi)`. |
| 2 | firmware/test/test_threshold.c:55 (renommage) et :84 (`RUN_TEST` commenté) | Le test `test_EF04_configured_hysteresis_is_used` est renommé `xtest_…` et son `RUN_TEST` est commenté (« flaky, à revoir »). C'est précisément le test qui détecte le constat n°1. Il n'est pas « flaky » : le code est déterministe (aucune horloge, aucun aléa). La CI est verte parce qu'on a retiré le test qui échouait. | Liste de contrôle « tests (ajouté / modifié / désactivé) » ; EF-04 | **Bloquant** | Réactiver le test, ne jamais désactiver un test pour faire passer la CI ; si un test est réellement instable, ouvrir un rapport de défaut dédié. |
| 3 | firmware/src/node.c:24 (`node_set_id`) | `strncpy(node_id, id, TN_ID_LEN)` : la ligne `node_id[TN_ID_LEN - 1u] = '\0'` est supprimée. Si `strlen(id) >= TN_ID_LEN`, `node_id` n'est pas terminé par `'\0'` ; les `memcpy`/`snprintf("%s")` suivants lisent hors du tampon. Régression de sûreté mémoire, sans rapport avec l'hystérésis. | Liste de contrôle « chaînes et tampons (terminaison, longueurs) » ; périmètre | **Bloquant** | Rétablir la copie sur `TN_ID_LEN - 1u` et la terminaison explicite ; retirer ce changement de cette MR. |
| 4 | firmware/src/threshold.c:31-38 | l.32 `ev = THR_CLEAR, above = 0u;` : opérateur virgule ; `if`/`else` sans accolades aux l.34, 36 et 38 ; l.31 expression `high_centi - 500` signalée par l'addon (parenthèses de précédence). L'addon MISRA signale 12.1 (l.31) et 15.6 (l.34, 36, 38) mais **pas** la virgule (12.3) : celle-ci n'a été vue qu'à la lecture. | MISRA C:2012 12.3 (virgule), 15.6 (corps de contrôle entre accolades) ; 12.1 (parenthèses de précédence) | Majeur | Remettre les accolades, une instruction par ligne. |
| 5 | firmware/src/threshold.c:38 et titre / message du commit | (a) Le test `above >= THR_CONSECUTIVE` est sorti du bloc `if (filtered > high)` : refactorisation non demandée, équivalente ici (le compteur est remis à 0 dans le `else`) mais qui change la structure d'une fonction P1/sûreté sans test dédié. (b) Le diff touche `node.c` alors que le titre annonce l'hystérésis. (c) Message « fix hysteresis » : ni type(portée), ni référence EF-04/DEF. | Liste de contrôle « périmètre », « message de commit » | Mineur | Séparer en commits distincts (`fix(threshold): … (EF-04)`, `fix(node): …`), ou retirer la refactorisation. |

## Rapports de défaut

### DR-TP4-01 — L'hystérésis configurée est ignorée dans `threshold_step`
- **Commit** : `1e6862c` (branche `fix/EF-04-hysteresis`)
- **Environnement** : VM TP0, gcc 13, `make -C firmware test`
- **Étapes** : 1. `threshold_set(6000, 300);` 2. `threshold_step(5700, true)` 3. `threshold_step(5699, true)`
- **Attendu** : étape 2 → `THR_NONE` ; étape 3 → `THR_CLEAR` (5699 < 6000 − 300)
- **Obtenu** : étape 3 → `THR_NONE` (le seuil de levée reste 6000 − 500 = 5500)
- **Sévérité** : haute (une alerte reste active trop longtemps / hystérésis non configurable) — **Priorité** : haute, bloque la fusion
- **Références** : EF-04, liste de contrôle « constantes » ; test masqué par le constat n°2
- **Pièce jointe** (sortie réelle du test ad hoc) :
  ```
  step(5700,true)=0 (attendu 0 = THR_NONE)
  step(5699,true)=0 (attendu 2 = THR_CLEAR)
  ```

### DR-TP4-02 — Chaîne `node_id` potentiellement non terminée
- **Commit** : `1e6862c`
- **Environnement** : VM TP0, gcc 13, `make -C firmware SAN=1 test` (ASan/UBSan)
- **Étapes** : `node_init(6000, 500);` `node_set_id("ABCDEFGH");` (8 caractères = `TN_ID_LEN`), puis réception d'une commande valide (`set_period`, CRC correct) : `node_poll_commands()` → `send_ack()` → `snprintf("%s", node_id)` (`node.c:159-160`)
- **Attendu** : identifiant tronqué à 7 caractères et terminé par `'\0'`, soit `{"id":"ABCDEFG","ack":"set_period","result":"ok","reason":""}` (comportement observé sur `main`)
- **Obtenu** : `node_id` n'est pas terminé par `'\0'` ; `snprintf` lit hors du tableau de 8 octets (ASan : lecture de 9 octets)
- **Sévérité** : haute (corruption/lecture mémoire) — **Priorité** : haute
- **Références** : liste de contrôle « chaînes et tampons » ; périmètre de la MR
- **Pièce jointe** (sortie réelle, extrait) :
  ```
  READ of size 9 at 0x602b4e458ba8 thread T0
      #2 ... in snprintf
      #3 ... in send_ack src/node.c:159
      #4 ... in node_poll_commands src/node.c:196
      #5 ... in main /tmp/adhoc2.c:15
  ```
  Sur `main` (référence) : `ack = {"id":"ABCDEFG","ack":"set_period","result":"ok","reason":""}`, aucune erreur.

## Décision

**Demander des changements.** Merci pour la volonté de corriger EF-04, mais la MR introduit une régression (hystérésis en dur, n°1) que la suite ne détecte plus parce que le test correspondant est désactivé (n°2), et supprime une terminaison de chaîne (n°3) hors du périmètre annoncé.
Merci de réactiver le test, de rétablir `(high_centi - hyst_centi)` et la terminaison dans `node_set_id`, de corriger le style MISRA (12.3, 15.6) et de scinder les commits ; je revois dès la prochaine version.

## Q1 — Quelle technique a révélé quoi ?

| # | Révélé par | Ne pouvait pas le révéler |
|---|---|---|
| 1 constante en dur | lecture avec liste de contrôle ; test ad hoc `threshold_set(6000,300)` | cppcheck (code valide) ; la suite telle quelle (seul test concerné désactivé, et la valeur par défaut 500 masque l'erreur) |
| 2 test désactivé | lecture du diff (partie tests) | tous les outils et la CI : verts par construction, on a retiré le test qui échoue |
| 3 terminaison `strncpy` | lecture (« chaînes et tampons ») ; test dynamique avec ASan et un id de 8 caractères (lecture de 9 octets dans `send_ack`) | tests existants (jamais d'id ≥ `TN_ID_LEN`) ; cppcheck « classique » d'après l'énoncé |
| 4 virgule / accolades | addon MISRA pour 15.6 (l.34, 36, 38) et 12.1 (l.31) ; **lecture seule** pour la virgule (12.3, non signalée par l'outil) | tests et analyse dynamique (le comportement est correct) |
| 5 périmètre / commit | lecture uniquement | tout outil automatique |

**Conclusion** : une CI verte prouve seulement que les tests écrits passent, pas que la modification est correcte. Le statique (revue, outils) voit ce que l'exécution ne voit pas (style, norme, périmètre, test supprimé) ; le dynamique voit ce que la lecture rate facilement (comportement avec d'autres valeurs). Les techniques sont complémentaires et aucune ne suffit seule.
