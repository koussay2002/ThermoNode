# TP0 — Réponses aux questions

## Q1 — À quoi sert `firmware/src/hal.h` ? Pourquoi trois implémentations ?

`firmware/src/hal.h` est l'interface (le contrat) qui abstrait le matériel
(ADC, GPIO, timer) pour le code applicatif. Elle déclare les fonctions que
le reste du firmware peut appeler sans connaître les détails matériels.

Trois implémentations existent dans `firmware/hal/` :

- **cible** (`hal_stm32_stub.c`) : pour la vraie carte STM32G031K8, utilisée
  par `make -C firmware size-check` (compilation croisée Cortex-M0+).
- **simulateur** (`hal_sim.c`) : pour exécuter le firmware sur PC dans
  l'environnement d'intégration Docker (`node-sim`).
- **bouchon / stub** : pour les tests unitaires Unity sur PC, sans avoir
  besoin de matériel.

Cela répond à l'exigence **C-02** (« logique applicative testable hors cible
via une couche HAL ») et permet de tout tester sans carte physique.

## Q2 — « 0 Failures » ⇒ le firmware est-il correct ?

Non. Le squelette ne contient que **2 tests** (sur 75 prévus à terme) et
la couverture de code est de **2,5 %**. Le message « 0 Failures » prouve
uniquement que le **harnais de test** fonctionne correctement (Unity + make),
pas que le firmware respecte toutes les exigences (EF-01 à EF-08,
ENF-01 à ENF-06).

Cela illustre les principes du Chapitre 1 : *« l'absence de preuve n'est pas
la preuve de l'absence »* et *« un test qui passe ne prouve pas l'absence de
bug »*. La qualité se prouve par un ensemble de preuves (tests, couverture,
analyse statique, porte mémoire, CI), pas par un seul indicateur.

## Q3 — Pourquoi la passerelle ne plante pas sur `{pas du json` ?

La passerelle (`gateway/handler.py`) traite chaque trame dans un
`try / except FrameError`. Sur une trame invalide (JSON malformé, CRC
incorrect, champ manquant…), elle :

1. incrémente `frames_rejected` et `rejected_<raison>` (métriques Prometheus) ;
2. loggue un `WARNING` ;
3. **retourne `None` sans interrompre le service**.

Les logs montrent d'ailleurs :
