# Analyse statique TP4 — pylint (gateway)

## Etat initial (avant corrections)

- Commande : `pylint gateway` (depuis ~/tn/gateway, venv actif)
- Note : 9.05/10
- Messages : 18

| Code | Nom | Nb |
|---|---|---|
| W0613 | unused-argument | 5 |
| C0115 | missing-class-docstring | 5 |
| R0917 | too-many-positional-arguments | 2 |
| R0913 | too-many-arguments | 2 |
| W0511 | fixme | 1 |
| C0321 | multiple-statements | 1 |
| C0207 | use-maxsplit-arg | 1 |
| C0103 | invalid-name | 1 |

## Corrections (un commit par regle)

1. 474f565 style(gateway): docstrings de classes (C0115) — 5 docstrings
2. e78d57c style(gateway): separe instructions multiples (C0321)
3. 79642af style(gateway): maxsplit explicite (C0207)
4. 389642b style(gateway): arguments inutilises prefixes _ (W0613) — 5 args
5. 086d579 style(gateway): supprime invalid-name justifie (C0103)
6. 77ac083 style(gateway): retire TODO obsolete (W0511)
7. 612bf5d chore(gateway): supprime too-many-arguments justifie (R0913/R0917)

## Etat final

- Note : 10.00/10
- Messages : 0

## Tableau avant / apres

| Metrique | Avant | Apres |
|---|---|---|
| Note pylint | 9.05/10 | 10.00/10 |
| Messages | 18 | 0 |

## Suppressions justifiees

| Regle | Emplacement | Justification |
|---|---|---|
| C0103 | gateway/health.py do_GET | nom impose par BaseHTTPRequestHandler |
| R0913 / R0917 | gateway/storage.py insert() | signature imposee par le schema measures (6 colonnes) |
| R0913 / R0917 | gateway/mqtt_client.py _on_connect() | callback impose par paho-mqtt |

## Note pedagogique

La note a brievement baisse (9.05 -> 8.73) apres C0115 : pylint normalise la note sur le nombre de lignes, donc retirer 5 messages fait peser plus lourd les 13 restants. Le vrai indicateur est le nombre de messages (18 -> 0).
