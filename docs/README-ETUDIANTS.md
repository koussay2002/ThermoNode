# Dépôt étudiant ThermoNode (squelette)

Ce dépôt contient le firmware et la passerelle **avec des défauts volontaires** que vos tests doivent révéler
(TP 3), puis que vous corrigerez sous revue (TP 4), sous pipeline (TP 5) et documenterez (projet).
Les tests fournis sont volontairement incomplets ; `test/hal_double.c` contient des TODO.

Commandes : `make -C firmware test | coverage | static | size-check | sim`, `cd gateway && pytest tests/unit`,
`docker compose -f ci/compose.test.yaml up -d --build --wait`.
