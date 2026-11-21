# Dérogations MISRA C:2012 — ThermoNode

| ID | Règle | Fichier | Justification | Approbation |
|---|---|---|---|---|
| DEV-001 | 11.4 (conversion pointeur ↔ entier) | hal/hal_stm32.c (via STM32Cube) | Accès aux registres mappés en mémoire par la bibliothèque HAL du fabricant | Responsable qualité |
| DEV-002 | 21.6 (`<stdio.h>`) | src/frame.c, src/node.c | `snprintf` utilisé pour la sérialisation JSON avec contrôle de la longueur ; alternative (sérialisation manuelle) jugée plus risquée | Responsable qualité |
| DEV-003 | 21.6 (`<stdio.h>`) | hal/hal_sim.c | Code de simulation hôte, non embarqué | — |
| DEV-004 | 15.5 (Advisory, point de sortie unique) | src/* | Retours anticipés dans les gardes de validation : évitent une imbrication profonde et gardent la lecture linéaire | Revue de MR |
| DEV-005 | 18.4 (Advisory, arithmétique de pointeur) | src/frame.c | Construction et analyse du JSON dans un tampon ; les tailles (cap, len) sont vérifiées avant les accès ; l'alternative par indices est plus verbeuse sans gain de sûreté | Revue de MR |
| DEV-006 | 21.10, 21.1 et 2.5 (`<time.h>`, identifiant réservé `_POSIX_C_SOURCE`, macro sans usage dans le code) | src/main.c (bloc `#ifdef TN_SIM`) | Code de simulation sur hôte uniquement, jamais compilé pour la cible Cortex-M0+ | Revue de MR |
| DEV-007 | 8.9 (Advisory, portée d'objet) | src/filter.c, src/fault.c, src/ringbuf.c | Variables `static` qui portent l'état d'un module, partagé entre plusieurs fonctions du module (reset, step) | Revue de MR |
